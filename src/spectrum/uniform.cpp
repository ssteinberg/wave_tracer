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
#include <wt/spectrum/uniform.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::spectrum;


scene::element::info_t uniform_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "uniform", {
        { "value", attributes::make_scalar(avg_spectral_pwr) },
    });
}


std::unique_ptr<spectrum_real_t> uniform_t::load(std::string id, 
                                             scene::loader::loader_t* loader, 
                                             const scene::loader::node_t& node, 
                                             const wt::wt_context_t &context) {
    const auto avg_pwr = stof_strict(node["constant"]);
    auto krange = range_t<wavenumber_t>::positive();

    for (auto& item : node.children_view()) {
    try {
        if (item.name() == "range") {
            // read range
            range_t<wavelength_t> wlrange;
            parse_range(item["value"], wlrange);
            krange = { 
                .min = wavelen_to_wavenum(wlrange.max), 
                .max = wavelen_to_wavenum(wlrange.min),
            };
        } 
        else
            logger::cwarn()
                << loader->node_description(item)
                << "(uniform spectrum loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(uniform spectrum loader) " + std::string{ exp.what() }, item);
    }
    }

    if (avg_pwr<0)
        throw scene_loading_exception_t("(uniform spectrum loader) a non-negative spectral power must be provided", node);
    if ((krange & range_t<wavenumber_t>::positive()).empty() ||
         !range_t<wavenumber_t>::positive().contains(krange))
        throw scene_loading_exception_t("(uniform spectrum loader) range must be non-empty and non-negative", node);

    return std::make_unique<uniform_t>(std::move(id), avg_pwr, krange);
}
