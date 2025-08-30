/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>

#include <wt/scene/element/attributes.hpp>
#include <wt/scene/loader/loader.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/integrator/plt_bdpt/vertex.hpp>
#include <wt/integrator/plt_bdpt/plt_bdpt_detail.hpp>
#include <wt/integrator/plt_bdpt/plt_bdpt.hpp>

#include <wt/ads/ads.hpp>
#include <wt/scene/scene.hpp>
#include <wt/sensor/film/film.hpp>
#include <wt/bsdf/bsdf.hpp>

#include <wt/sampler/uniform.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::integrator;


thread_local plt_bdpt::arena_t bdpt_arena;

plt_bdpt_t::plt_bdpt_t(const wt_context_t &ctx,
                       std::string id,
                       plt_bdpt_t::options_t opts) noexcept
    : integrator_t(std::move(id)),
      options(opts)
{}

void plt_bdpt_t::integrate(const integrator_context_t& ctx,
                           const sensor::block_handle_t& block,
                           const vec3u32_t& sensor_element,
                           std::uint32_t samples_per_element) const noexcept {
    // grab thread local memory arena
    auto* arena = &bdpt_arena;

    // use a uniform sampler for path sampling
    static auto uniform_sampler = sampler::uniform_t{};
    auto& path_sampling_sampler = uniform_sampler;

    for (std::uint32_t sample=0; sample<samples_per_element; ++sample) {
        // draw spectral sample and emitter sample
        const auto emitter_wavenumber = ctx.scene->sample_emitter_and_spectrum_and_source_beam(ctx.sensor);

        const auto& emitter_sample    = emitter_wavenumber.emitter_sample;
        const auto& wavenumber_sample = emitter_wavenumber.wavenumber;
        const auto& k = wavenumber_sample.k;

        // spectral (importance) sampling weight:
        // for discrete spectral samples, division by the sampling probability mass.
        // for continuos spectra, importance sample over all probability densities to sample this k.
        const wavenumber_t recp_spectral_pd = emitter_wavenumber.wavenumber.wpd.is_discrete() ?
            f_t(1) / wavenumber_density_t{ emitter_wavenumber.wavenumber.wpd.mass() * u::mm } :
            f_t(1) / ctx.scene->sum_spectral_pdf_for_all_emitters(ctx.sensor, k);

        // draw sensor sample
        const auto sensor_sample = ctx.sensor->sample(ctx.scene->sampler(), sensor_element, k);

        assert(m::isfinite(emitter_sample.beam.intensity()));
        assert(m::isfinite(sensor_sample.beam.intensity()));

        arena->sensor_vertices.clear();
        arena->emitter_vertices.clear();
        arena->fraunhofer_fsd_bsdfs.clear();

        // generate a sensor and emitter subpaths
        plt_bdpt::generate_sensor_subpath(arena->sensor_vertices,
                                          &arena->fraunhofer_fsd_bsdfs,
                                          fraunhofer_fsd_sampler.get(),
                                          options,
                                          sensor_sample,
                                          ctx, path_sampling_sampler);
        plt_bdpt::generate_emitter_subpath(arena->emitter_vertices,
                                           &arena->fraunhofer_fsd_bsdfs,
                                           fraunhofer_fsd_sampler.get(),
                                           options,
                                           emitter_wavenumber,
                                           ctx, path_sampling_sampler);

        auto L = radiant_flux_stokes_t::unpolarized(radiant_flux_t::zero());

        const auto& wpd = wavenumber_sample.wpd;
        const auto k_density = wpd.is_discrete() ? 
            wavenumber_density_t{ wpd.mass() / wavenumber_t::unit } : 
            wpd.density();

        // TODO: BDPT spectral MIS

        // integrate connections
        // t - sensor subpath
        // s - emitter subpath
        for (int t=0; t<=arena->sensor_vertices.size(); ++t) 
        for (int s=0; s<=arena->emitter_vertices.size(); ++s) {
            const auto depth = t+s-2;
            if ((t==1 && s==1) || depth<0) continue;
            if (!options.emitter_direct && s==1) continue;
            if (!options.sensor_direct  && t==1) continue;
            if (depth>options.max_depth) break;

            // connect
            auto ret = plt_bdpt::connect_subpaths(arena, ctx, options, 
                                                  s,t, path_sampling_sampler);
            if (ret.L.intensity()<=zero)
                continue;

            // MIS
            wavenumber_t mis;
            if (options.MIS) {
                mis = plt_bdpt::bdpt_compute_mis_weight(arena, ctx, options,
                                                        s,t, ret) *
                      recp_spectral_pd;
            } else {
                mis = 1 / (f_t(s+t+1) * k_density);
            }
            assert(m::isfinite(mis) && mis>=zero);

            // accumulate or splat to light image for direct-to-sensor connections
            const auto flux_sample = (radiant_flux_stokes_t)(ret.L * mis);
            if (t>1) L += flux_sample;
            else {
                const auto element = *ret.sensor_element_sample;
                ctx.sensor->splat_direct(ctx.film_surface,
                                         element,
                                         flux_sample,
                                         k);
            }
        }

        // splat to block
        ctx.sensor->splat(block,
                          sensor_sample.element,
                          L,
                          k);
    }
}


scene::element::info_t plt_bdpt_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "plt_bdpt", {
        { "max depth",        attributes::make_scalar(options.max_depth) },
        { "MIS",              attributes::make_scalar(options.MIS) },
        { "FSD",              attributes::make_scalar(options.FSD) },
        { "Russian Roulette", attributes::make_scalar(options.RR) },
    });
}

std::shared_ptr<integrator_t> plt_bdpt_t::load(const std::string& id,
                                               scene::loader::loader_t* loader, 
                                               const scene::loader::node_t& node,
                                               const wt::wt_context_t &context) {
    plt_bdpt_t::options_t opts{};

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::read_attribute(item,"max_depth",opts.max_depth) &&
            !scene::loader::read_attribute(item,"MIS",opts.MIS) &&
            !scene::loader::read_attribute(item,"FSD",opts.FSD) &&
            !scene::loader::read_attribute(item,"russian_roulette",opts.RR) &&
            !scene::loader::read_attribute(item,"sensor_direct_sampling",opts.sensor_direct) &&
            !scene::loader::read_attribute(item,"emitter_direct_sampling",opts.emitter_direct))
            logger::cwarn()
                << loader->node_description(item)
                << "(integrator loader) Unqueried node \"" << item["name"] << "\"" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(plt bdpt integrator loader) " + std::string{ exp.what() }, item);
    }
    }

    auto ptr = std::make_shared<plt_bdpt_t>( 
        context,
        id,
        opts
    );

    if (!context.renderer_force_ray_tracing) {
        loader->enqueue_loading_task(ptr.get(), "fsd_sampler", [ptr, id, &context]() {
            ptr->fraunhofer_fsd_sampler =
                std::make_unique<fraunhofer::fsd_sampler::fsd_sampler_t>(id+"_fsd_sampler", context);
        });
    }

    return ptr;
}
