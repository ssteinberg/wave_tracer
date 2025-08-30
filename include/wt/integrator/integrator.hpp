/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <string>
#include <memory>

#include <wt/wt_context.hpp>

#include <wt/scene/element/scene_element.hpp>
#include <wt/sampler/sampler.hpp>
#include <wt/ads/ads.hpp>

#include <wt/sensor/block/block.hpp>
#include <wt/sensor/sensor_sample.hpp>
#include <wt/sensor/sensor_flags.hpp>

namespace wt::integrator {

struct integrator_context_t;

/**
 * @brief Generic interface for light transport integrators.
 */
class integrator_t : public scene::scene_element_t {
public:
    static constexpr std::string scene_element_class() noexcept { return "integrator"; }

public:
    integrator_t(std::string id) : scene_element_t(std::move(id)) {}
    integrator_t(integrator_t&&) = default;
    virtual ~integrator_t() noexcept = default;

    /**
     * @brief Indicates how the integrator writes to the sensor, and which capabilities are expected from the sensor (or its underlying film).
     */
    [[nodiscard]] virtual sensor::sensor_write_flags_e sensor_write_flags() const noexcept = 0;

    /**
     * @brief Integrates light transport in the scene, given a sensor element and samples count.
     *        The arguments `block` and `sensor_element` are only used when `sensor_write_flags()` sets `wt::sensor::sensor_write_flags_e::writes_block_splats`, otherwise are ignored.
     * 
     * @param block block to write samples to
     * @param sensor_element sensor element that `block` belongs to
     * @param samples_per_element sample count to use for this element
     */
    virtual void integrate(const integrator_context_t& ctx,
                           const sensor::block_handle_t& block,
                           const vec3u32_t& sensor_element,
                           std::uint32_t samples_per_element) const noexcept = 0;

public:
    static std::shared_ptr<integrator_t> load(
            const std::string& id, 
            scene::loader::loader_t* loader, 
            const scene::loader::node_t& node, 
            const wt::wt_context_t &context);
};

}

#include "integrator_context.hpp"
