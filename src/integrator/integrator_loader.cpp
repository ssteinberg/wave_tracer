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

#include <memory>
#include <wt/util/logger/logger.hpp>

#include <wt/math/common.hpp>

#include <wt/integrator/plt_bdpt/plt_bdpt.hpp>
#include <wt/integrator/plt_path/plt_path.hpp>

using namespace wt;
using namespace wt::integrator;


std::shared_ptr<integrator_t> integrator_t::load(
        const std::string& id,
        scene::loader::loader_t* loader,
        const scene::loader::node_t& node,
        const wt::wt_context_t &context) {
    const std::string& type = node["type"];

    if (type=="plt_bdpt")
        return plt_bdpt_t::load(id, loader, node, context);
    else if (type=="plt_path")
        return plt_path_t::load(id, loader, node, context);

    throw scene_loading_exception_t("(integrator loader) Unrecognized integrator type", node);
}
