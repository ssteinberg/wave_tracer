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

#include <wt/emitter/emitter.hpp>
#include <wt/emitter/area.hpp>
#include <wt/emitter/directional.hpp>
#include <wt/emitter/point.hpp>
#include <wt/emitter/spot.hpp>

#include <wt/math/common.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace emitter;


std::unique_ptr<emitter_t> emitter_t::load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context) {
    const std::string type = node["type"];

    if (type=="area")
        return area_t::load(std::move(id), loader, node, context);
    else if (type=="directional")
        return directional_t::load(std::move(id), loader, node, context);
    else if (type=="point")
        return point_t::load(std::move(id), loader, node, context);
    else if (type=="spot")
        return spot_t::load(std::move(id), loader, node, context);

    throw scene_loading_exception_t("(emitter loader) unrecognized emitter type", node);
}
