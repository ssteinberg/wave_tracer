/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/wt_context.hpp>
#include <wt/sensor/film/defs.hpp>
#include <wt/sensor/sensor.hpp>

#include <wt/math/common.hpp>

namespace wt {

class wt_context;
class scene_t;
namespace ads { class ads_t; }

namespace integrator {

struct integrator_context_t {
    const wt_context_t* wtcontext;
    const scene_t* scene;
    const ads::ads_t* ads;

    const sensor::sensor_t* sensor;
    sensor::film_storage_handle_t* film_surface;
};

}
}
