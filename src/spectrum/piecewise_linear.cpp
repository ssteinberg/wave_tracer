/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <vector>
#include <optional>
#include <algorithm>

#include <wt/scene/loader/node.hpp>
#include <wt/scene/loader/node_readers.hpp>
#include <wt/spectrum/piecewise_linear.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::spectrum;


scene::element::info_t piecewise_linear_t::description() const {
    using namespace scene::element;

    attributes::wavelength_map_t::container_t bins;
    for (const auto& v : dist)
        bins.emplace(wavenum_to_wavelen(v.x / u::mm), attributes::make_scalar(v.y));

    return info_for_scene_element(*this, "piecewise_linear", {
        { "power", attributes::make_scalar(power()) },
        { "bins", attributes::make_wavelength_map(std::move(bins)) },
    });
}


std::unique_ptr<spectrum_real_t> piecewise_linear_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    std::vector<vec2_t> values;
    std::optional<f_t> scale;

    for (auto& item : node.children_view()) {
    try {
        if (item.name() == "bin") {
            const auto wl = parse_wavelength(item["wavelength"]);
            values.emplace_back(
                u::to_inv_mm(wavelen_to_wavenum(wl)),
                stof_strict(item["value"])
            );
            if (values.back().x<=0 || values.back().y<0)
                throw scene_loading_exception_t("(piecewise_linear spectrum loader) wavelength must be positive and value must be non-negative", item);
        }
        else if (!scene::loader::read_attribute(item, "scale", scale))
            logger::cwarn()
                << loader->node_description(item)
                << "(piecewise_linear spectrum loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(piecewise_linear spectrum loader) " + std::string{ exp.what() }, item);
    }
    }

    if (values.size()<2)
        throw scene_loading_exception_t("(piecewise_linear spectrum loader) at least 2 spectrum values must be provided", node);
    if (scale && *scale<0)
        throw scene_loading_exception_t("(piecewise_linear spectrum loader) 'scale' must be non-negative", node);

    if (scale) {
        for (auto& v : values)
            v.y *= *scale;
    }

    std::ranges::sort(values, [](const auto& lhs, const auto& rhs) { return lhs.x<rhs.x; });

    return std::make_unique<piecewise_linear_t>(std::move(id), piecewise_linear_distribution_t{ std::move(values) });
}
