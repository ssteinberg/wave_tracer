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
#include <wt/spectrum/complex_uniform.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::spectrum;


scene::element::info_t complex_uniform_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "complex_constant", {
        { "value", attributes::make_scalar(val) },
    });
}


std::unique_ptr<spectrum_t> complex_uniform_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    const auto& avg_pwr = parse_complex_strict(node["constant"]);
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
                << "(complex uniform spectrum loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(complex uniform spectrum loader) " + std::string{ exp.what() }, item);
    }
    }

    if ((krange & range_t<wavenumber_t>::positive()).empty() ||
         !range_t<wavenumber_t>::positive().contains(krange))
        throw scene_loading_exception_t("(complex uniform spectrum loader) range must be non-empty and non-negative", node);

    return std::make_unique<complex_uniform_t>(std::move(id), avg_pwr, krange);
}
