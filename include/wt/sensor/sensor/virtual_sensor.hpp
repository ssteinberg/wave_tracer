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

#include <wt/sensor/sensor_sample.hpp>
#include <wt/beam/beam.hpp>
#include <wt/math/range.hpp>

namespace wt::sensor {

/**
 * @brief Generic interface for virtual sensors (sensors that have a virtual geometry associated with them).
 */
class virtual_coverage_sensor_t {
public:
    virtual_coverage_sensor_t() noexcept = default;
    virtual_coverage_sensor_t(virtual_coverage_sensor_t&&) = default;

    /**
     * @brief Integrate a radiation beam over the sensor.
     * @param beam radiation beam
     * @param range beam propagation distance from its origin to integrate over
     */
    [[nodiscard]] virtual std::optional<sensor_direct_connection_t> Si(
            const spectral_radiant_flux_beam_t& beam,
            const pqrange_t<>& range) const noexcept = 0;
    
    /**
     * @brief Total number of sensor elements (e.g., pixels), per dimension. Returns 1 for unused dimensions.
     */
    [[nodiscard]] virtual vec3u32_t sensor_elements() const noexcept = 0;

    /**
     * @brief Returns the world position of an element on this virtual sensor.
     * @param element sensor element id (integer part) and fractional offset (fractional part)
     */
    [[nodiscard]] virtual pqvec3_t position_for_element(const sensor::sensor_element_sample_t& element) const noexcept = 0;

    /**
     * @brief Returns the film element for a world position on this virtual sensor.
     */
    [[nodiscard]] virtual sensor::sensor_element_sample_t element_for_position(const pqvec3_t& wp) const noexcept = 0;
};

}
