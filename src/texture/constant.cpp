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

#include <wt/texture/constant.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace texture;


scene::element::info_t constant_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "constant", {
        { "spectrum", attributes::make_element(spectrum.get()) },
    });
}

std::unique_ptr<texture_t> constant_t::load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context) {
    std::shared_ptr<spectrum::spectrum_real_t> spectrum;

    try {
    for (auto& item : node.children_view()) {
        if (!scene::loader::load_scene_element(item, spectrum, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(constant texture loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    }
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(constant texture loader) " + std::string{ exp.what() }, node);
    }
    
    if (!spectrum)
        throw scene_loading_exception_t("(constant texture loader) A nested real spectrum must be provided", node);

    return std::make_unique<constant_t>(std::move(id), std::move(spectrum));
}
