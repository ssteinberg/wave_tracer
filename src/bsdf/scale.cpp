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

#include <wt/bsdf/scale.hpp>
#include <wt/texture/scale.hpp>

#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace bsdf;


scene::element::info_t scale_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "scale", {
        { "scale", attributes::make_element(scale.get()) },
        { "nested", attributes::make_element(nested.get()) },
    });
}


std::unique_ptr<bsdf_t> scale_t::load(std::string id, 
                                      scene::loader::loader_t* loader, 
                                      const scene::loader::node_t& node, 
                                      const wt::wt_context_t &context) {
    std::shared_ptr<texture::texture_t> scale;
    std::shared_ptr<bsdf_t> bsdf;

    // a constant scale can also be given inline as scale="<scale>", used as a shorthand.
    const auto& shorthand_scale = node["scale"];
    const bool has_shorthand_scale = shorthand_scale.length()>0;
    if (has_shorthand_scale) {
        const auto s = stof_strict(shorthand_scale);

        auto spectrum = std::make_unique<spectrum::uniform_t>(id + "_scale_spectrum", s);
        scale = std::make_unique<texture::constant_t>(id + "_scale_texture", std::move(spectrum));
    }

    for (auto& item : node.children_view()) {
    try {
        if (!has_shorthand_scale &&
            scene::loader::load_texture_element(item, "scale", scale, loader, context))
            continue;
        if (!scene::loader::load_scene_element(item, bsdf, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(scale bsdf loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(scale bsdf loader) " + std::string{ exp.what() }, item);
    }
    }

    if (!bsdf || !scale)
        throw scene_loading_exception_t("(scale bsdf loader) 'scale' bsdf must contain a nested bsdf and a scaling texture", node);

    return std::make_unique<scale_t>(std::move(id), std::move(scale), std::move(bsdf));
}
