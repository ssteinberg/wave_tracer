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
#include <wt/scene/element/attributes.hpp>
#include <wt/wt_context.hpp>

#include <wt/texture/complex_constant.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace texture;


scene::element::info_t complex_constant_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "complex_constant", {
        { "spectrum", attributes::make_element(spectrum.get()) },
    });
}


std::shared_ptr<complex_t> complex_constant_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    std::shared_ptr<spectrum::spectrum_t> spectrum;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_scene_element(item, spectrum, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(complex constant texture loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(complex constant texture loader) " + std::string{ exp.what() }, item);
    }
    }
    
    if (!spectrum)
        throw scene_loading_exception_t("(complex constant texture loader) A nested spectrum must be provided", node);

    return std::make_unique<complex_constant_t>(std::move(id), std::move(spectrum));
}
