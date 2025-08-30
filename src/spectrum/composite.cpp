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
#include <wt/spectrum/composite.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::spectrum;


scene::element::info_t composite_t::description() const {
    using namespace scene::element;

    using range = range_t<wavelength_t, range_inclusiveness_e::left_inclusive>;

    attributes::wavelength_range_map_t::container_t spectra;
    for (const auto& c : this->spectra)
        spectra.emplace(range{ wavenum_to_wavelen(c.first.max), wavenum_to_wavelen(c.first.min) },
                        attributes::make_element(c.second.get()));

    return info_for_scene_element(*this, "composite", {
        { "power", attributes::make_scalar(power()) },
        { "spectra", attributes::make_wavelength_range_map(std::move(spectra)) },
    });
}


std::unique_ptr<spectrum_real_t> composite_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    map_t spectra;

    for (auto& item : node.children_view()) {
    try {
        if (item.name() == "bin") {
            // read range
            range_t<wavelength_t> wlrange;
            parse_range(item["wavelength_range"], wlrange);
            map_range_t range = { 
                .min = wavelen_to_wavenum(wlrange.max), 
                .max = wavelen_to_wavenum(wlrange.min),
            };

            // read spectrum
            std::shared_ptr<spectrum_real_t> spectrum;
            for (const auto& c : item.children_view()) {
                if (!scene::loader::load_scene_element(c, spectrum, loader, context))
                    logger::cwarn()
                        << loader->node_description(item)
                        << "(composite spectrum loader) unqueried node type " << c.name() 
                        << " (\"" << c["name"] << "\")"
                        << '\n';
            }

            if (!spectrum)
                throw scene_loading_exception_t("(composite spectrum loader) no child spectrum provided in bin", item);
                
            spectra.emplace(range, std::move(spectrum));
        } else {
            logger::cwarn()
                << loader->node_description(item)
                << "(composite spectrum loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
        }
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(composite spectrum loader) " + std::string{ exp.what() }, item);
    }
    }

    if (spectra.empty())
        throw scene_loading_exception_t("(composite spectrum loader) no child spectrum provided", node);

    for (const auto& s1 : spectra)
    for (const auto& s2 : spectra) {
        if (&s1==&s2) break;
        if (!(s1.first & s2.first).empty())
            throw scene_loading_exception_t("(composite spectrum loader) child spectra wavelength ranges must be non-overlapping", node);
    }

    return std::make_unique<composite_t>(std::move(id), std::move(spectra));
}
