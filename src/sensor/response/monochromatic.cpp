/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/wt_context.hpp>

#include <wt/sensor/response/response.hpp>
#include <wt/sensor/response/monochromatic.hpp>

#include <wt/math/common.hpp>

#include <wt/scene/element/attributes.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace sensor::response;


monochromatic_t::monochromatic_t(std::string id, 
                                 std::shared_ptr<tonemap_t> tonemap,
                                 std::shared_ptr<spectrum::spectrum_real_t> spectrum)
    : response_t(std::move(id), std::move(tonemap)),
      spectrum(std::move(spectrum))
{}


scene::element::info_t monochromatic_t::description() const {
    using namespace scene::element;
    auto ret = info_for_scene_element(*this, "monochromatic", {
        { "spectrum", attributes::make_element(spectrum.get()) },
    });
    if (this->get_tonemap())
        ret.attribs.emplace("tonemap operator", attributes::make_element(this->get_tonemap()));
    return ret;
}

std::unique_ptr<response_t> monochromatic_t::load(std::string id, 
                                                  scene::loader::loader_t* loader, 
                                                  const scene::loader::node_t& node, 
                                                  const wt::wt_context_t &context) {
    std::shared_ptr<spectrum::spectrum_real_t> spectrum;
    std::shared_ptr<tonemap_t> tonemap;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_scene_element(item, spectrum, loader, context) &&
            !scene::loader::load_scene_element(item, tonemap, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(monochromatic response loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(monochromatic response loader) " + std::string{ exp.what() }, item);
    }
    }

    if (!spectrum)
        throw scene_loading_exception_t("(monochromatic response loader) 'spectrum' must be provided", node);
    if (!tonemap)
        tonemap = std::make_shared<tonemap_t>(id + "__respfunc");

    return std::make_unique<monochromatic_t>( 
        std::move(id), 
        std::move(tonemap),
        std::move(spectrum)
    );
}
