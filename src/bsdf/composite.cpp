/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/scene/loader/node.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/bsdf/bsdf.hpp>
#include <wt/bsdf/composite.hpp>

#include <wt/wt_context.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace bsdf;


scene::element::info_t composite_t::description() const {
    using namespace scene::element;

    using range = range_t<wavelength_t, range_inclusiveness_e::left_inclusive>;

    attributes::wavelength_range_map_t::container_t bsdfs;
    for (const auto& b : this->bsdfs)
        bsdfs.emplace(range{ wavenum_to_wavelen(b.first.max), wavenum_to_wavelen(b.first.min) },
                      attributes::make_element(b.second.get()));

    return info_for_scene_element(*this, "composite", {
        { "bsdfs", attributes::make_wavelength_range_map(std::move(bsdfs)) },
    });
}


std::unique_ptr<bsdf_t> composite_t::load(std::string id, 
                                          scene::loader::loader_t* loader, 
                                          const scene::loader::node_t& node, 
                                          const wt::wt_context_t &context) {
    map_t bsdfs;

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

            // read bsdf
            std::shared_ptr<bsdf_t> bsdf;
            for (const auto& c : item.children_view()) {
                if (!scene::loader::load_scene_element(c, bsdf, loader, context))
                    logger::cwarn()
                        << loader->node_description(item)
                        << "(composite bsdf loader) unqueried node type "
                        << c.name() << " (\"" << c["name"] << "\")" 
                        << '\n';
            }

            if (!bsdf)
                throw scene_loading_exception_t("(composite bsdf loader) no child bsdf provided in bin", item);
                
            bsdfs.emplace(range, std::move(bsdf));
        }
        else
            logger::cwarn()
                << loader->node_description(item)
                << "(diffuse bsdf loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(diffuse bsdf loader) " + std::string{ exp.what() }, item);
    }
    }

    if (bsdfs.empty())
        throw scene_loading_exception_t("(composite bsdf loader) no child bsdf provided", node);

    for (const auto& s1 : bsdfs)
    for (const auto& s2 : bsdfs) {
        if (&s1==&s2) break;
        if (!(s1.first & s2.first).empty())
            throw scene_loading_exception_t("(composite bsdf loader) child bsdfs wavelength ranges must be non-overlapping", node);
    }
    

    return std::make_unique<composite_t>(std::move(id), std::move(bsdfs));
}
