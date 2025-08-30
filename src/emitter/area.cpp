/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>

#include <wt/scene/loader/node.hpp>
#include <wt/scene/loader/node_readers.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/emitter/area.hpp>
#include <wt/texture/texture.hpp>
#include <wt/spectrum/colourspace/RGB/RGB.hpp>

#include <wt/interaction/intersection.hpp>
#include <wt/scene/shape.hpp>

#include <wt/sampler/measure.hpp>
#include <wt/math/common.hpp>
#include <wt/math/util.hpp>

#include <wt/util/logger/logger.hpp>

#include <wt/sampler/uniform.hpp>

using namespace wt;
using namespace wt::emitter;


spectral_radiant_flux_stokes_t emitter::area_t::Li(
        const importance_flux_beam_t& Sbeam,
        const intersection_surface_t* surface) const noexcept {
    const auto dn = m::dot(-Sbeam.dir(),surface->ng());
    if (dn<=0)
        return {};
    
    // TODO: this is an approximation for small beams, integrate correctly over the emitter's shape.

    // source beam from impact position
    const auto& k = Sbeam.k();
    const auto Ibeam = Le(
        ray_t{ surface->wp, -Sbeam.dir() },
        k, surface) / dn;

    // use flipped frame for sourcing (unpolarized source)
    return beam::integrate_beams(Sbeam, Ibeam);
}

emitter_sample_t emitter::area_t::sample(sampler::sampler_t& sampler, 
                                         const wavenumber_t k) const noexcept {
    const auto& pos_sample = sample_position(sampler);
    auto d = sampler.cosine_hemisphere();
    const auto dn = d.z;

    assert(!!pos_sample.surface);

    d = pos_sample.surface->geo.to_world(d);
    const auto P = ray_t{ pos_sample.p, d };

    const auto dpd = solid_angle_density_t{ sampler.cosine_hemisphere_pdf(dn) / u::ang::sr };
    const auto ppd = pos_sample.ppd.density_or_zero();
    auto recp_pdf = f_t(1) / (dpd * ppd);
    if (dpd * ppd == zero)
        recp_pdf = {};
    assert(m::isfinite(recp_pdf) && recp_pdf>=zero);

    const auto beam = Le(P, k, &*pos_sample.surface) * recp_pdf;

    return emitter_sample_t{
        .beam = beam,

        .ppd = ppd,
        .dpd = dpd,

        .surface = pos_sample.surface,
    };
}

emitter_direct_sample_t emitter::area_t::sample_direct(sampler::sampler_t& sampler,
                                                       const pqvec3_t& wp,
                                                       const wavenumber_t k) const noexcept {
    const auto& pos_sample = sample_position(sampler);
    assert(!!pos_sample.surface);
    assert(pos_sample.p != wp);

    const auto d = m::normalize(wp - pos_sample.p);
    const auto dpd = pdf_direct(wp,
                                ray_t{ pos_sample.p,d },
                                &*pos_sample.surface).density();
    const auto recp_dpd = dpd>zero ? 1/dpd : solid_angle_t::zero();

    const auto P = ray_t{ pos_sample.p, d };
    const auto beam = Le(P, k, &*pos_sample.surface) *
                      recp_dpd / u::ang::sr;

    return emitter_direct_sample_t{
        .emitter = this,
        .dpd = dpd,
        .beam = beam,
        .surface = pos_sample.surface,
    };
}

position_sample_t emitter::area_t::sample_position(sampler::sampler_t& sampler) const noexcept {
    // if we do not use the triangle sampler, use the shape's uniform sampler
    if (!use_triangle_sampling_data || !sampling_data)
        return shape->sample_position(sampler);

    const auto ret = sampling_data->sample(this, sampler);
    return {
        .p = ret.surface.wp,
        .ppd = ret.ppd,
        .surface = ret.surface
    };
}

area_sampling_pd_t emitter::area_t::pdf_position(const pqvec3_t& p,
                                                 const intersection_surface_t* surface) const noexcept {
    // if we do not use the triangle sampler, use the shape's uniform sampler
    if (!use_triangle_sampling_data || !sampling_data)
        return shape->pdf_position(p);

    assert(surface);
    return sampling_data->pdf(this, *surface);
}

solid_angle_sampling_pd_t emitter::area_t::pdf_direction(
        const pqvec3_t& p,
        const dir3_t& dir,
        const intersection_surface_t* surface) const noexcept {
    assert(surface);
    const auto dn = m::max<f_t>(0, m::dot(dir, surface->ng()));
    return solid_angle_density_t{ sampler::sampler_t::cosine_hemisphere_pdf(dn) / u::ang::sr };
}

solid_angle_sampling_pd_t emitter::area_t::pdf_direct(
        const pqvec3_t& wp,
        const ray_t& r,
        const intersection_surface_t* surface) const noexcept {
    const auto ppd = pdf_position(r.o, surface);

    const auto l2 = m::length2(wp-r.o);
    const auto dn = m::max<f_t>(0, m::dot(r.d, surface->ng()));
    const auto recp_dn = (dn>0 ? 1/dn : 0) / u::ang::sr;

    return solid_angle_density_t{ ppd.density_or_zero() * l2 * recp_dn };
}


void emitter::area_t::set_shape(const wt_context_t& ctx, const shape_t* shape) {
    assert(!this->shape);
    this->shape = shape;
    construct_triangle_sampling_data(ctx);
}

void emitter::area_t::construct_triangle_sampling_data(const wt_context_t& ctx) {
    // check if we need sampling data
    std::unique_lock l(*samplig_data_mutex);
    {
        if (!radiance || !shape || sampling_data ||
            !use_triangle_sampling_data)
            return;
    }

    /* construct sampling data...
     */

    logger::cout(verbosity_e::info)
        << "Constructing barycentric sampling data for textured area emitter <" << get_id() << ">..." << '\n';

    // TODO: add support for non RGB emission textures. E.g., allow a single-channel luminance modulator with an explicit spectrum?

    // compute working resolution
    constexpr f_t max_resolution = 512;
    const auto rad_res = m::max_element(radiance->resolution());
    const auto res = m::min(rad_res, max_resolution);
    // TODO: res might be smaller than native resolution. Mipmap?

    // loop over all triangles in the emitter's mesh
    // for each, loop over barycentric coordinates and query the texture, thereby building an emission per barycentrics distribution
    // do so in parallel

    const auto& tris = shape->get_mesh().get_tris();
    std::vector<std::future<std::pair<f_t, triangle_sampling_data_t>>> fs;
    fs.reserve(tris.size());
    for (const auto& t : tris) {
        fs.emplace_back(ctx.threadpool->enqueue([&t, res, this]() {
            assert(t.uv);   // no UV?!
            if (!t.uv)
                return std::make_pair(
                        f_t(0),
                        triangle_sampling_data_t{ .uv_dist=discrete_distribution_t<f_t>(std::vector<f_t>{ 0 }) });

            const auto& uv = *t.uv;
            const auto tarea = util::tri_surface_area(t.p[0], t.p[1], t.p[2]);

            const auto max_uv_dist = m::max(
                    m::length(uv[1]-uv[0]), m::length(uv[2]-uv[0]), m::length(uv[2]-uv[1]));
            const auto texels  = (std::uint32_t)m::ceil(res * max_uv_dist);
            const auto bary_step   = f_t(1) / texels;

            std::vector<f_t> texels_I;
            texels_I.reserve((texels+1)*(texels+2)/2);

            // loop over barycentrics
            f_t I = 0;
            std::size_t count = 0;
            for (int b=0; b<texels; ++b)
            for (int a=0; a<=b; ++a) {
                const auto alpha = a*bary_step + f_t(.5)*bary_step;
                const auto beta  = 1 - (b*bary_step + f_t(.5)*bary_step);
                assert(alpha+beta<=1 && alpha>=0 && beta>=0);
                const auto tuv = alpha*uv[0] + beta*uv[1] + m::max<f_t>(0,1-alpha-beta)*uv[2];

                const auto t = radiance->get_RGBA(texture::texture_query_t{ .uv=tuv });
                const auto lum = colourspace::luminance(vec3_t{ t });

                texels_I.emplace_back(lum);
                I += lum;
                ++count;
            }
            assert(count == texels*(texels+1)/2);

            return std::make_pair(
                    I / f_t(count) * u::to_m2(tarea),
                    triangle_sampling_data_t{
                        .texels = texels,
                        .recp_texels = bary_step,
                        .texel_to_area_density = f_t(1) / (m::sqr(bary_step) * tarea),
                        .uv_dist = discrete_distribution_t<f_t>(std::move(texels_I)),
                    }
            );
        }));
    }
    
    std::vector<f_t> triangle_powers;
    triangle_powers.reserve(tris.size());
    triangle_sampler_t triangle_samplers;
    f_t totalI = 0;
    for (auto& f : fs) {
        auto ret = f.get();
        triangle_powers.emplace_back(ret.first);
        triangle_samplers.emplace_back(std::move(ret.second));

        totalI += ret.first;
    }

    if (totalI==0) {
        logger::cerr() << 
            "(area emitter <" + get_id() + ">) radiance texture is zero everywhere, or does not implement get_RGBA()." << '\n';
    }

    sampling_data = std::make_unique<sampling_data_t>(sampling_data_t{
        .triangle_dist = discrete_distribution_t<f_t>(std::move(triangle_powers)),
        .triangle_samplers = std::move(triangle_samplers),
    });
}


wt::emitter::area_t::sampling_data_t::triangle_sample_t wt::emitter::area_t::sampling_data_t::sample(
        const area_t* emitter,
        sampler::sampler_t& sampler) const noexcept {
    const auto r = sampler.r4();

    // sample triangle
    const auto tid  = triangle_dist.icdf(r.x);
    const auto tpdf = triangle_dist.pdf(tid);

    // sample barycentrics
    const auto& tsampler = triangle_samplers[tid];
    const auto bary_sample = (std::uint32_t)tsampler.uv_dist.icdf(r.y);
    const auto bary_pdf    = tsampler.uv_dist.pdf(bary_sample);

    const auto b = (std::uint32_t)m::ceil(-f_t(.5) + m::sqrt(f_t(.25) + f_t(2*(bary_sample+1)))) - 1;
    const auto a = bary_sample - b*(b+1)/2;

    assert(bary_sample>=b);
    assert(a<tsampler.texels && b<tsampler.texels);

    const auto& bary_step = tsampler.recp_texels;
    const auto alpha = m::clamp01(a*bary_step + r.z*bary_step);
    const auto beta  = m::clamp<f_t>(1 - (b*bary_step + r.w*bary_step), 0, 1-alpha);
    const auto bary = barycentric_t{ vec2_t{ alpha, beta } };
    
    return {
        // convert to area density
        .ppd = tpdf * bary_pdf * tsampler.texel_to_area_density,
        .surface = intersection_surface_t(emitter->shape, tid, bary),
    };

}
area_density_t wt::emitter::area_t::sampling_data_t::pdf(
        const area_t* emitter,
        const intersection_surface_t& surface) const noexcept {
    // intersection with wrong shape??
    assert(surface.shape == emitter->shape);
    if (surface.shape != emitter->shape)
        return {};

    const auto& bary = surface.bary.coords();
    const auto& tid  = surface.mesh_tri_idx;
    const auto& tsampler = triangle_samplers[tid];

    // triangle pdf
    const auto tpdf = triangle_dist.pdf(tid);

    // calculate uv pdf
    const auto a = (std::uint32_t)m::round(bary.x * tsampler.texels - f_t(.5));
    const auto b = (std::uint32_t)m::round((1 - bary.y) * tsampler.texels - f_t(.5));
    const auto offset = b*(b+1)/2 + a;
    const auto bary_pdf = tsampler.uv_dist.pdf(offset);

    return tpdf * bary_pdf * tsampler.texel_to_area_density;
}


scene::element::info_t emitter::area_t::description() const {
    using namespace scene::element;
    return radiance ?
        info_for_scene_element(*this, "area", {
            { "radiance", attributes::make_element(radiance.get()) },
            { "average_spectrum", attributes::make_element(average_radiance.get()) },
            { "power", attributes::make_scalar(this->power(range_t<wavenumber_t>::positive())) },
        }) :
        info_for_scene_element(*this, "area", {
            { "average_spectrum", attributes::make_element(average_radiance.get()) },
            { "power", attributes::make_scalar(this->power(range_t<wavenumber_t>::positive())) },
        });
}

std::unique_ptr<emitter_t> emitter::area_t::load(std::string id,
                                                 scene::loader::loader_t* loader,
                                                 const scene::loader::node_t& node,
                                                 const wt::wt_context_t &context) {
    std::shared_ptr<texture::texture_t> radiance;
    f_t scale = 1;
    f_t phase_space_extent_scale = 1;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_texture_element(item, "radiance", radiance, loader, context) &&
            !scene::loader::read_attribute(item, "scale", scale) &&
            !scene::loader::read_attribute(item, "phase_space_extent_scale", phase_space_extent_scale))
            logger::cwarn()
                << loader->node_description(item)
                << "(area emitter loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(area emitter loader) " + std::string{ exp.what() }, item);
    }
    }

    if (!radiance)
        throw scene_loading_exception_t("(area emitter loader) a 'radiance' texture or real spectrum must be provided", node);

    auto area = std::make_unique<area_t>( 
        std::move(id), 
        nullptr, nullptr,
        scale,
        phase_space_extent_scale
    );

    loader->enqueue_loading_task(area.get(), area->get_id(),
        [area=area.get(), radiance=std::move(radiance), loader, &node, &context]() {
        // wait for any texture loaders to complete
        loader->complete_loading_tasks_for_resource(radiance.get());

        // extract texture, if any, and average emission spectrum from input radiance
        std::shared_ptr<texture::texture_t> radiance_texture;
        auto average_emission_spectrum = radiance->mean_spectrum();

        if (radiance->needs_interaction_footprint())
            throw scene_loading_exception_t("(area emitter loader) the radiance texture cannot require interaction footprint.", node);
        if (!average_emission_spectrum)
            throw scene_loading_exception_t("(area emitter loader) the radiance texture must provide mean_spectrum().", node);

        if (!radiance->is_constant()) {
            // we have a non constant texture
            radiance_texture = radiance;
        }

        area->radiance = std::move(radiance_texture);
        area->average_radiance = std::move(average_emission_spectrum);
        area->construct_triangle_sampling_data(context);
    });

    return area;
}
