/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/wt_context.hpp>
#include <wt/scene/loader/loader.hpp>
#include <wt/scene/loader/node.hpp>

#include <wt/interaction/surface_profile/surface_profile.hpp>
#include <wt/interaction/surface_profile/dirac.hpp>
#include <wt/interaction/surface_profile/fractal.hpp>
#include <wt/interaction/surface_profile/gaussian.hpp>

#include <wt/math/common.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace surface_profile;


std::unique_ptr<surface_profile_t> surface_profile_t::load(std::string id, 
                                                           scene::loader::loader_t* loader, 
                                                           const scene::loader::node_t& node, 
                                                           const wt::wt_context_t &context) {
    const std::string& type = node["type"];

    if (type=="dirac")
        return dirac_t::load(std::move(id), loader, node, context);
    else if (type=="fractal")
        return fractal_t::load(std::move(id), loader, node, context);
    else if (type=="gaussian")
        return gaussian_t::load(std::move(id), loader, node, context);

    throw scene_loading_exception_t("(surface_profile loader) unrecognized surface_profile type", node);
}
