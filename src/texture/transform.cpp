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

#include <wt/texture/transform.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace texture;


scene::element::info_t texture::transform_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "transform", {
        { "texture", attributes::make_element(tex.get()) },
        { "translate", attributes::make_vector(translate) },
        { "M", attributes::make_matrix(M) },
    });
}


std::unique_ptr<texture_t> texture::transform_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    std::shared_ptr<texture_t> tex;
    mat2_t M = mat2_t{ 1 };
    vec2_t translate = {};

    for (auto& item : node.children_view()) {
    try {
        if (item.name() == "matrix")
            M = parse_matrix2(item["value"]);
        else if (item.name() == "translate")
            translate = parse_vec2(item["value"]);
        else if (!scene::loader::load_scene_element(item, tex, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(transform texture loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(transform texture loader) " + std::string{ exp.what() }, item);
    }
    }
    
    if (!tex)
        throw scene_loading_exception_t("(transform texture loader) A nested texture must be provided", node);

    auto ret = std::make_unique<transform_t>( 
        std::move(id), 
        std::move(tex), 
        M, translate
    );

    loader->register_resource_dependency(ret.get(), ret->tex.get());

    return ret;
}
