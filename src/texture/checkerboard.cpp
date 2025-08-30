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

#include <wt/texture/checkerboard.hpp>
#include <wt/math/common.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace texture;


scene::element::info_t checkerboard_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "checkerboard", {
        { "colour1", attributes::make_element(col1.get()) },
        { "colour2", attributes::make_element(col2.get()) },
    });
}


std::unique_ptr<texture_t> checkerboard_t::load(std::string id, 
                                                scene::loader::loader_t* loader, 
                                                const scene::loader::node_t& node, 
                                                const wt::wt_context_t &context) {
    std::shared_ptr<texture_t> col1, col2;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_texture_element(item, "colour1", col1, loader, context) &&
            !scene::loader::load_texture_element(item, "colour2", col2, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(checkerboard texture loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(checkerboard texture loader) " + std::string{ exp.what() }, item);
    }
    }
    
    if (!col1)
        col1 = std::make_shared<constant_t>(id+"_default_colour1", 0);
    if (!col2)
        col2 = std::make_shared<constant_t>(id+"_default_colour2", 1);

    auto ret = std::make_unique<checkerboard_t>(std::move(id), std::move(col1), std::move(col2));

    loader->register_resource_dependency(ret.get(), ret->col1.get());
    loader->register_resource_dependency(ret.get(), ret->col2.get());

    return ret;
}
