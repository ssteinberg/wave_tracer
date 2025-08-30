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

#include <wt/sensor/response/response.hpp>
#include <wt/sensor/response/monochromatic.hpp>
#include <wt/sensor/response/multichannel.hpp>
#include <wt/sensor/response/RGB.hpp>
#include <wt/sensor/response/XYZ.hpp>

#include <wt/math/common.hpp>
#include <wt/scene/loader/node_readers.hpp>

using namespace wt;
using namespace sensor::response;


std::unique_ptr<response_t> response_t::load(std::string id, 
                                             scene::loader::loader_t* loader, 
                                             const scene::loader::node_t& node, 
                                             const wt::wt_context_t &context) {
    const std::string& type = node["type"];

    if (type=="monochromatic")
        return monochromatic_t::load(std::move(id), loader, node, context);
    else if (type=="multichannel")
        return multichannel_t::load(std::move(id), loader, node, context);
    else if (type=="RGB")
        return RGB_t::load(std::move(id), loader, node, context);
    else if (type=="XYZ")
        return XYZ_t::load(std::move(id), loader, node, context);

    throw scene_loading_exception_t("(response function loader) unrecognized response function type", node);
}
