/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/interaction/surface_profile/dirac.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

#include <wt/scene/loader/node.hpp>
#include <wt/scene/element/attributes.hpp>
#include <wt/scene/loader/node_readers.hpp>

using namespace wt;
using namespace wt::surface_profile;


scene::element::info_t dirac_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "dirac");
}

std::unique_ptr<surface_profile_t> dirac_t::load(std::string id, 
                                                 scene::loader::loader_t* loader, 
                                                 const scene::loader::node_t& node, 
                                                 const wt::wt_context_t& context) {
    for (auto& item : node.children_view()) {
    try {
        logger::cwarn()
            << loader->node_description(item)
            << "(dirac surface_profile loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(dirac surface_profile loader) " + std::string{ exp.what() }, item);
    }
    }

    return std::make_unique<dirac_t>(std::move(id));
}
