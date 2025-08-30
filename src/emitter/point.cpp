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
#include <wt/scene/loader/node_readers.hpp>

#include <wt/emitter/point.hpp>

#include <wt/math/common.hpp>
#include <wt/math/transform/transform.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::emitter;


emitter_sample_t point_t::sample(sampler::sampler_t& sampler,
                                 const wavenumber_t k) const noexcept {
    const auto d = sampler.uniform_sphere();
    const auto P = ray_t{ position,d };

    const auto beam = Le(P, k) *
                      (m::four_pi * u::ang::sr);

    return emitter_sample_t{
        .beam = beam,

        .ppd = area_sampling_pd_t::discrete(1),
        .dpd = solid_angle_density_t{ sampler.uniform_sphere_pdf() / u::ang::sr },
    };
}

emitter_direct_sample_t point_t::sample_direct(sampler::sampler_t& sampler,
                                               const pqvec3_t& wp,
                                               const wavenumber_t k) const noexcept {
    const auto p = position;
    const auto dl = wp-p;
    const auto recp_dist2 = 1 / (area_t)m::length2(dl);
    const auto d = dir3_t{ dl * m::sqrt(recp_dist2) };

    const auto beam = Le(ray_t{ position, d }, k) *
                      recp_dist2;

    return emitter_direct_sample_t{
        .emitter = this,
        .dpd = solid_angle_sampling_pd_t::discrete(1),
        .beam = beam,
    };
}


scene::element::info_t emitter::point_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "point", {
        { "position", attributes::make_vector(position) },
        { "radiant intensity", attributes::make_element(spectrum.get()) },
            { "power", attributes::make_scalar(this->power(range_t<wavenumber_t>::positive())) },
    });
}


std::unique_ptr<emitter_t> point_t::load(std::string id, 
                                         scene::loader::loader_t* loader, 
                                         const scene::loader::node_t& node, 
                                         const wt::wt_context_t &context) {
    std::shared_ptr<spectrum::spectrum_real_t> radiant_intensity;
    std::optional<pqvec3_t> position;
    std::optional<transform_t> to_world;
    f_t phase_space_extent_scale = 1;
    std::optional<length_t> extent;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_scene_element(item, "radiant_intensity", radiant_intensity, loader, context) &&
            !scene::loader::read_attribute(item, "position", position) &&
            !scene::loader::read_attribute(item, "spatial_extent", extent) &&
            !scene::loader::load_transform(item, "to_world", to_world, loader) &&
            !scene::loader::read_attribute(item, "phase_space_extent_scale", phase_space_extent_scale))
            logger::cwarn()
                << loader->node_description(item)
                << "(point emitter loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(point emitter loader) " + std::string{ exp.what() }, item);
    }
    }

    if (!radiant_intensity)
        throw scene_loading_exception_t("(point emitter loader) a real 'radiant_intensity' spectrum must be provided", node);
    if (!!position && !!to_world)
        throw scene_loading_exception_t("(point emitter loader) 'to_world' and 'position' can not be both specified", node);
    if (!!extent && extent.value()<=zero)
        throw scene_loading_exception_t("(point emitter loader) 'spatial_extent' cannot be vanishing or negative", node);

    if (!position) {
        if (!to_world)
            to_world = transform_t{};
        position = (*to_world)(pqvec3_t::zero(), transform_point);
    }

    return std::make_unique<point_t>( 
        std::move(id), 
        *position, 
        std::move(radiant_intensity),
        extent,
        phase_space_extent_scale
    );
}
