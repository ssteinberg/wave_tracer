/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>

#include <wt/scene/loader/node.hpp>
#include <wt/sensor/sensor/perspective.hpp>
#include <wt/sensor/sensor/virtual_plane_sensor.hpp>

#include <wt/math/transform/transform.hpp>

#include <wt/scene/loader/node_readers.hpp>

using namespace wt;
using namespace wt::sensor;

std::shared_ptr<sensor_t> sensor_t::load(std::string id, 
                                         scene::loader::loader_t* loader, 
                                         const scene::loader::node_t& node, 
                                         const wt::wt_context_t &context) {
    const std::string& type = node["type"];
    const bool polarimetric = stob_strict(node["polarimetric"], false);

    if (type=="perspective") {
        if (polarimetric)
            return perspective_polarimetric_t::load(std::move(id), loader, node, context);
        else
            return perspective_scalar_t::load(std::move(id), loader, node, context);
    }
    else if (type=="virtual_plane") {
        if (polarimetric)
            throw scene_loading_exception_t("(sensor loader) polarimetric sensor not supported for this sensor type", node);
        return virtual_plane_sensor_t::load(std::move(id), loader, node, context);
    }

    throw scene_loading_exception_t("(sensor loader) unrecognized sensor type", node);
}
