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

#include <wt/texture/mix.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace texture;


scene::element::info_t mix_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "mix", {
        { "texture1", attributes::make_element(texture1.get()) },
        { "texture2", attributes::make_element(texture2.get()) },
        { "mix", attributes::make_element(mix.get()) },
    });
}


std::unique_ptr<texture_t> mix_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    std::shared_ptr<texture_t> tex1, tex2, mix;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_texture_element(item, "texture1", tex1, loader, context) &&
            !scene::loader::load_texture_element(item, "texture2", tex2, loader, context) &&
            !scene::loader::load_texture_element(item, "mix", mix, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(mix texture loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(mix texture loader) " + std::string{ exp.what() }, item);
    }
    }
    
    if (!tex1 || !tex2 || !mix)
        throw scene_loading_exception_t("(mix texture loader) Nested textures 'texture1', 'texture2' and 'mix' must be provided", node);

    auto ret = std::make_unique<mix_t>(std::move(id), std::move(tex1), std::move(tex2), std::move(mix));

    loader->register_resource_dependency(ret.get(), ret->texture1.get());
    loader->register_resource_dependency(ret.get(), ret->texture2.get());
    loader->register_resource_dependency(ret.get(), ret->mix.get());

    return ret;
}
