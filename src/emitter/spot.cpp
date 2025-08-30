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
#include <wt/scene/loader/node_readers.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/emitter/spot.hpp>
#include <wt/interaction/intersection.hpp>

#include <wt/math/common.hpp>
#include <wt/math/transform/transform.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::emitter;


emitter_sample_t spot_t::sample(sampler::sampler_t& sampler, 
                                const wavenumber_t k) const noexcept {
    const auto cutoff_solid_angle = m::two_pi * (1-cos_cutoff);

    const auto local_wo = sampler.uniform_cone(cutoff_solid_angle);
    const auto wo = to_world(local_wo);

    const auto w    = compute_falloff(local_wo);
    const auto dpd  = solid_angle_density_t{ sampler.uniform_cone_pdf(cutoff_solid_angle) / u::ang::sr };
    const auto beam = Le(ray_t{ position(), wo }, k) * 
                      w / dpd;

    return emitter_sample_t{
        .beam = beam,

        .ppd = area_sampling_pd_t::discrete(1),
        .dpd = dpd,
    };
}

emitter_direct_sample_t spot_t::sample_direct(sampler::sampler_t& sampler,
                                              const pqvec3_t& wp,
                                              const wavenumber_t k) const noexcept {
    const auto p = position();
    const auto dl = wp-p;
    const auto recp_dist2 = 1 / (area_t)m::length2(dl);
    const auto d = dir3_t{ dl * m::sqrt(recp_dist2) };

    const auto local_wo = to_world.inverse()(d);
    const auto w = compute_falloff(local_wo);
    const auto beam = Le(ray_t{ position(), d }, k) *
                      w * recp_dist2;

    return emitter_direct_sample_t{
        .emitter = this,
        .dpd = solid_angle_sampling_pd_t::discrete(1),
        .beam = beam,
    };
}


scene::element::info_t emitter::spot_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "spot", {
        { "position",          attributes::make_vector(position()) },
        { "direction",         attributes::make_vector(mean_direction()) },
        { "falloff",           attributes::make_scalar(falloff) },
        { "cutoff",            attributes::make_scalar(cutoff) },
        { "radiant intensity", attributes::make_element(spectrum.get()) },
            { "power", attributes::make_scalar(this->power(range_t<wavenumber_t>::positive())) },
    });
}

std::unique_ptr<emitter_t> spot_t::load(std::string id, 
                                        scene::loader::loader_t* loader, 
                                        const scene::loader::node_t& node, 
                                        const wt::wt_context_t &context) {
    std::shared_ptr<spectrum::spectrum_real_t> radiant_intensity;
    transform_t to_world;
    angle_t cutoff = 20 * u::ang::deg;
    std::optional<angle_t> falloff;
    f_t phase_space_extent_scale = 1;
    std::optional<length_t> extent;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_scene_element(item, "radiant_intensity", radiant_intensity, loader, context) &&
            !scene::loader::load_transform(item, "to_world", to_world, loader) &&
            !scene::loader::read_attribute(item, "spatial_extent", extent) &&
            !scene::loader::read_attribute(item, "cutoff_angle", cutoff) &&
            !scene::loader::read_attribute(item, "beam_width", falloff) &&
            !scene::loader::read_attribute(item, "phase_space_extent_scale", phase_space_extent_scale))
            logger::cwarn()
                << loader->node_description(item)
                << "(spot emitter loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(spot emitter loader) " + std::string{ exp.what() }, item);
    }
    }

    if (!radiant_intensity)
        throw scene_loading_exception_t("(spot emitter loader) a real 'radiant_intensity' spectrum must be provided", node);
    if (cutoff<=0*u::ang::rad)
        throw scene_loading_exception_t("(spot emitter loader) 'cutoff_angle' must be a positive value", node);
    if (m::cos(cutoff)==1)
        throw scene_loading_exception_t("(spot emitter loader) 'cutoff_angle' too small", node);
    if (!!falloff && *falloff>=cutoff)
        throw scene_loading_exception_t("(spot emitter loader) 'beam_width' must be less than 'cutoff_angle'", node);
    if (!!extent && extent.value()<=zero)
        throw scene_loading_exception_t("(spot emitter loader) 'spatial_extent' cannot be vanishing or negative", node);

    if (!falloff)
        falloff = cutoff * f_t(.75);

    return std::make_unique<spot_t>(
        std::move(id),
        std::move(radiant_intensity),
        cutoff, *falloff,
        to_world,
        extent,
        phase_space_extent_scale
    );
}
