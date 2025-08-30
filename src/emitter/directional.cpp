/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <optional>

#include <wt/scene/loader/node.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/emitter/directional.hpp>

#include <wt/math/common.hpp>
#include <wt/math/transform/transform.hpp>

#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::emitter;


emitter_sample_t directional_t::sample(sampler::sampler_t& sampler,
                                       const wavenumber_t k) const noexcept {
    assert(target_radius>zero);

    // draw position on target
    const auto p   = sampler.concentric_disk() * target_radius;
    const auto wp  = world_centre() + frame.to_world(p);
    const auto ppd = recp_surface_area;
    const auto recp_ppd = surface_area;

    const auto P = ray_t{ wp + far*vec3_t{ dir_to_emitter },-dir_to_emitter };

    const auto beam = Le(P, k) *
                      recp_ppd;

    return emitter_sample_t{
        .beam = beam,

        .ppd = ppd,
        .dpd = solid_angle_sampling_pd_t::discrete(1),
    };
}

emitter_direct_sample_t directional_t::sample_direct(
        sampler::sampler_t& sampler,
        const pqvec3_t& wp,
        const wavenumber_t k) const noexcept {
    assert(target_radius>zero);

    const auto p  = pqvec2_t{ frame.to_local(wp-world_centre()) };
    const auto r2 = m::length2(p);
    const f_t scale = r2<=m::sqr(target_radius) ? 1 : 0;

    const auto targetwp = world_centre() + frame.to_world(p);
    const auto P = ray_t{ targetwp + far*vec3_t{ dir_to_emitter }, -dir_to_emitter };

    const auto beam = Le(P, k) *
                      scale / u::ang::sr;

    return emitter_direct_sample_t{
        .emitter = this,
        .dpd = solid_angle_sampling_pd_t::discrete(1),
        .beam = beam,
    };
}


scene::element::info_t emitter::directional_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "directional", {
        { "direction from emitter", attributes::make_vector(-dir_to_emitter) },
        { "irradiance", attributes::make_element(spectrum.get()) },
            { "power", attributes::make_scalar(this->power(range_t<wavenumber_t>::positive())) },
    });
}


std::unique_ptr<emitter_t> directional_t::load(std::string id, 
                                               scene::loader::loader_t* loader, 
                                               const scene::loader::node_t& node, 
                                               const wt::wt_context_t &context) {
    std::shared_ptr<spectrum::spectrum_real_t> irradiance;
    std::optional<transform_t> to_world;
    f_t phase_space_extent_scale = 1;
    solid_angle_t sa_at_traget = f_t(6.794e-5) * u::ang::sr;        // defaults to solid angle of sun on earth

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_scene_element(item, "irradiance", irradiance, loader, context) &&
            !scene::loader::load_transform(item, "to_world", to_world, loader) &&
            !scene::loader::read_attribute(item, "solid_angle", sa_at_traget) &&
            !scene::loader::read_attribute(item, "phase_space_extent_scale", phase_space_extent_scale))
            logger::cwarn()
                << loader->node_description(item)
                << "(directional emitter loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(directional emitter loader) " + std::string{ exp.what() }, item);
    }
    }

    if (!irradiance)
        throw scene_loading_exception_t("(directional emitter loader) a real 'irradiance' spectrum must be provided", node);
    if (sa_at_traget<=zero)
        throw scene_loading_exception_t("(directional emitter loader) 'solid_angle' cannot be vanishing or negative", node);

    dir3_t dir = { 0,0,-1 };
    if (to_world)
        dir = (*to_world)(dir);

    return std::make_unique<directional_t>(
        std::move(id),
        dir,
        std::move(irradiance),
        sa_at_traget,
        phase_space_extent_scale
    );
}
