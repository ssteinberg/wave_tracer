/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <optional>

#include <wt/sampler/density.hpp>

#include <wt/beam/beam.hpp>
#include <wt/interaction/intersection.hpp>

#include <wt/math/common.hpp>

namespace wt {

namespace sensor {

class sensor_t;

/**
 * @brief Generic sensor element sample information.
 */
struct sensor_element_sample_t {
    vec3u32_t element;
    vec3_t offset;
};

}

struct sensor_direct_sample_t {
    const sensor::sensor_t* sensor;

    importance_beam_t beam;

    /** @brief Density of direct connection (solid angle or discrete measure).
     */
    solid_angle_sampling_pd_t dpd;

    /** @brief Film element.
     */
    sensor::sensor_element_sample_t element;

    std::optional<intersection_surface_t> surface;
};

struct sensor_direct_connection_t {
    importance_beam_t beam;

    /** @brief Film element.
     */
    sensor::sensor_element_sample_t element;

    std::optional<intersection_surface_t> surface;
};

struct sensor_sample_t {
    const sensor::sensor_t* sensor;

    importance_flux_beam_t beam;

    /** @brief Position density.
     */
    area_sampling_pd_t ppd;
    /** @brief Direction density.
     */
    solid_angle_sampling_pd_t dpd;

    /** @brief Film element.
     */
    sensor::sensor_element_sample_t element;

    std::optional<intersection_surface_t> surface;
};

}
