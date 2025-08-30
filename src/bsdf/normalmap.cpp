/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/scene/loader/node.hpp>
#include <wt/wt_context.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/bsdf/normalmap.hpp>

#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace bsdf;


scene::element::info_t normalmap_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "normalmap", {
        { "normalmap", attributes::make_element(normalmap.get()) },
        { "nested", attributes::make_element(nested.get()) },
    });
}


std::unique_ptr<bsdf_t> normalmap_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    std::shared_ptr<texture::texture_t> normalmap;
    std::shared_ptr<bsdf_t> bsdf;
    bool flip_normal_map_tangent = false;

    for (auto& item : node.children_view()) {
        if (!scene::loader::load_scene_element(item, normalmap, loader, context) &&
            !scene::loader::load_scene_element(item, bsdf, loader, context) &&
            !scene::loader::read_attribute(item, "flip_tangent", flip_normal_map_tangent))
            logger::cwarn()
                << loader->node_description(item)
                << "(normalmap bsdf loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    }

    if (!normalmap)
        throw scene_loading_exception_t("(normalmap bsdf loader) 'normalmap' bsdf must contain a nested texture", node);
    if (!bsdf)
        throw scene_loading_exception_t("(normalmap bsdf loader) 'normalmap' bsdf must contain a nested bsdf", node);

    return std::make_unique<normalmap_t>(
        std::move(id), 
        std::move(normalmap), std::move(bsdf),
        flip_normal_map_tangent
    );
}
