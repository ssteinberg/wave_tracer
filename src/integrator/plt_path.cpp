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

#include <wt/integrator/plt_path/plt_path.hpp>
#include <wt/integrator/plt_path/plt_path_detail.hpp>

#include <wt/ads/ads.hpp>
#include <wt/scene/scene.hpp>
#include <wt/sensor/film/film.hpp>
#include <wt/bsdf/bsdf.hpp>

#include <wt/sampler/uniform.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::integrator;


plt_path_t::plt_path_t(const wt_context_t &ctx,
                       std::string id,
                       plt_path_t::options_t opts) noexcept
    : integrator_t(std::move(id)),
      options(opts)
{}

void plt_path_t::integrate(const integrator_context_t& ctx,
                           const sensor::block_handle_t& block,
                           const vec3u32_t& sensor_element,
                           std::uint32_t samples_per_element) const noexcept {
    if (options.transport_direction == bsdf::transport_e::forward) {
        for (std::uint32_t sample=0; sample<samples_per_element; ++sample)
            plt_path::integrate_forward(ctx, sensor_element, options);
    } else {
        for (std::uint32_t sample=0; sample<samples_per_element; ++sample)
            plt_path::integrate_backward(ctx, block, sensor_element, options);
    }
}


scene::element::info_t plt_path_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "plt_path", {
        { "max depth",        attributes::make_scalar(options.max_depth) },
        { "direction",        attributes::make_enum(options.transport_direction) },
        { "FSD",              attributes::make_scalar(options.FSD) },
        { "Russian Roulette", attributes::make_scalar(options.RR) },
    });
}

std::shared_ptr<integrator_t> plt_path_t::load(const std::string& id,
                                               scene::loader::loader_t* loader, 
                                               const scene::loader::node_t& node,
                                               const wt::wt_context_t &context) {
    plt_path_t::options_t opts{};
    std::optional<bsdf::transport_e> direction;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::read_attribute(item,"max_depth",opts.max_depth) &&
            !scene::loader::read_attribute(item,"FSD",opts.FSD) &&
            !scene::loader::read_attribute(item,"russian_roulette",opts.RR) &&
            !scene::loader::read_enum_attribute(item,"direction",direction))
            logger::cwarn()
                << loader->node_description(item)
                << "(integrator loader) Unqueried node \"" << item["name"] << "\"" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(plt path integrator loader) " + std::string{ exp.what() }, item);
    }
    }

    if (!direction)
        throw scene_loading_exception_t("(plt_path integrator loader) 'direction' must be specified", node);
    opts.transport_direction = *direction;

    return std::make_shared<plt_path_t>( 
        context,
        id,
        opts
    );
}
