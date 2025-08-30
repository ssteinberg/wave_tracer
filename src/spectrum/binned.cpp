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

#include <wt/scene/loader/node.hpp>
#include <wt/scene/loader/node_readers.hpp>
#include <wt/spectrum/binned.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::spectrum;


scene::element::info_t binned_t::description() const {
    using namespace scene::element;

    std::vector<attribute_ptr> bins;
    for (const auto& v : dist)
        bins.emplace_back(attributes::make_scalar(v));

    return info_for_scene_element(*this, "binned", {
        { "power", attributes::make_scalar(power()) },
        { "range", attributes::make_range(dist.range()) },
        { "bins", attributes::make_array(std::move(bins)) },
    });
}


std::unique_ptr<spectrum_real_t> binned_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    std::vector<f_t> values;
    std::optional<range_t<>> range;
    std::optional<f_t> scale;

    for (auto& item : node.children_view()) {
    try {
        if (item.name() == "bin") {
            values.emplace_back(
                stof_strict(item["value"])
            );
            if (values.back()<0)
                throw scene_loading_exception_t("(piecewise_linear spectrum loader) bin value must be non-negative", item);
        }
        else if (!scene::loader::read_attribute(item, "scale", scale) &&
                 !scene::loader::read_attribute(item, "range", range))
            logger::cwarn()
                << loader->node_description(item)
                << "(piecewise_linear spectrum loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(piecewise_linear spectrum loader) " + std::string{ exp.what() }, item);
    }
    }

    if (values.size()<2)
        throw scene_loading_exception_t("(piecewise_linear spectrum loader) at least 2 spectrum values must be provided", node);
    if (!range || range->length()<=0)
        throw scene_loading_exception_t("(piecewise_linear spectrum loader) a range with positive length must be provided", node);
    if (scale && *scale<0)
        throw scene_loading_exception_t("(piecewise_linear spectrum loader) 'scale' must be non-negative", node);

    if (scale) {
        for (auto& v : values)
            v *= *scale;
    }

    return std::make_unique<binned_t>(std::move(id), binned_piecewise_linear_distribution_t{ std::move(values),*range });
}
