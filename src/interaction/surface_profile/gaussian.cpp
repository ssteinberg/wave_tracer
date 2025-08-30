/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/interaction/surface_profile/gaussian.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

#include <wt/scene/loader/node.hpp>
#include <wt/scene/element/attributes.hpp>
#include <wt/scene/loader/node_readers.hpp>

using namespace wt;
using namespace wt::surface_profile;


scene::element::info_t gaussian_t::description() const {
    using namespace scene::element;

    auto ret = info_for_scene_element(*this, "gaussian");
    if (roughness_parametrized) {
        ret.attribs.emplace("roughness", attributes::make_element(roughness_tex.get()));
    } else {
        ret.attribs.emplace("surface variance", attributes::make_element(sigma_tex.get()));
    }
    return ret;
}

std::unique_ptr<surface_profile_t> gaussian_t::load(std::string id, 
                                                    scene::loader::loader_t* loader, 
                                                    const scene::loader::node_t& node, 
                                                    const wt::wt_context_t& context) {
    std::shared_ptr<texture::texture_t> roughness_tex;
    std::unique_ptr<texture::quantity_t<rms_t>> sigma_tex;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_texture_element(item, "roughness", roughness_tex, loader, context) &&
            !scene::loader::load_quantity_texture_element<rms_t>(id+"_sigma", item, "sigma", sigma_tex, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(gaussian surface_profile loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(gaussian surface_profile loader) " + std::string{ exp.what() }, item);
    }
    }

    const bool has_sigma = !!sigma_tex;
    const bool has_roughness = !!roughness_tex;
    if ((!has_sigma && !has_roughness) || (has_sigma && has_roughness))
        throw scene_loading_exception_t("(gaussian surface_profile loader) Either 'roughness' or 'sigma' must be provided", node);

    return has_roughness ?
        std::make_unique<gaussian_t>(std::move(id), std::move(roughness_tex)) :
        std::make_unique<gaussian_t>(std::move(id), std::move(sigma_tex));
}
