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
#include <wt/wt_context.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/texture/complex_container.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace texture;


scene::element::info_t complex_container_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "complex_container", {
        { "real", attributes::make_element(real_texture.get()) },
        { "imag", attributes::make_element(imag_texture.get()) },
    });
}


std::shared_ptr<complex_t> complex_container_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    std::shared_ptr<texture_t> real, imag;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_texture_element(item, "real", real, loader, context) &&
            !scene::loader::load_texture_element(item, "imag", imag, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(complex container texture loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(complex container texture loader) " + std::string{ exp.what() }, item);
    }
    }
    
    if (!real || !imag)
        throw scene_loading_exception_t("(complex container texture loader) Nested textures 'real' and 'imag' must be provided", node);

    auto ret = std::make_unique<complex_container_t>(std::move(id), std::move(real), std::move(imag));
    
    loader->register_resource_dependency(ret.get(), ret->real_texture.get());
    loader->register_resource_dependency(ret.get(), ret->imag_texture.get());

    return ret;
}
