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

#include <wt/sensor/block/block.hpp>
#include <wt/sensor/film/defs.hpp>
#include <wt/sensor/sensor_sample.hpp>
#include <wt/sensor/sensor_flags.hpp>
#include <wt/sensor/response/response.hpp>

#include <wt/beam/beam.hpp>
#include <wt/scene/element/scene_element.hpp>
#include <wt/sampler/sampler.hpp>
#include <wt/spectrum/spectrum.hpp>
#include <wt/interaction/polarimetric/stokes.hpp>

#include <wt/math/common.hpp>
#include <wt/math/shapes/ray.hpp>

#include <wt/wt_context.hpp>

namespace wt {

class scene_t;

namespace sensor {

/**
 * @brief Generic sensor interface. Sensors can be imaging or non-imaging.
 *        Sensor elements are partitioned into blocks; blocks are meant to be accessed in parallel by an integrator.
 */
class sensor_t : public scene::scene_element_t {
    friend class wt::scene_t;

public:
    static constexpr std::string scene_element_class() noexcept { return "sensor"; }

private:
    std::uint32_t samples_per_element;
    bool ray_trace;

public:
    sensor_t(std::string id, 
             std::uint32_t samples_per_element,
             bool ray_trace)
        : scene_element_t(std::move(id)),
          samples_per_element(samples_per_element),
          ray_trace(ray_trace)
    {}
    sensor_t(sensor_t&&) = default;
    virtual ~sensor_t() noexcept = default;

    [[nodiscard]] inline auto ray_trace_only() const noexcept { return ray_trace; }

    [[nodiscard]] virtual bool is_polarimetric() const noexcept = 0;

    [[nodiscard]] virtual const response::response_t* sensor_response() const noexcept = 0;

    /**
     * @brief Returns the beam phase-space extent for sourced beams from this sensor for a given wavenumber
     */
    [[nodiscard]] virtual beam::phase_space_extent_t sourcing_beam_extent(const wavenumber_t k) const noexcept = 0;

    /**
     * @brief Requested integrator samples per sensor element.
     */
    [[nodiscard]] inline auto requested_samples_per_element() const noexcept { return samples_per_element; }

    [[nodiscard]] virtual const spectrum::spectrum_real_t& sensitivity_spectrum() const noexcept = 0;


    /**
     * @brief Creates the sensor storage. Used as a render target for rendering.
     */
    [[nodiscard]] virtual std::unique_ptr<film_storage_handle_t> create_sensor_film(
            const wt::wt_context_t &context,
            sensor_write_flags_e flags) const noexcept = 0;

    /**
     * @brief Total number of sensor elements (e.g., pixels), per dimension. Returns 1 for unused dimensions.
     */
    [[nodiscard]] virtual vec3u32_t resolution() const noexcept = 0;

    /**
     * @brief Total number of parallel blocks available for rendering.
     */
    [[nodiscard]] virtual std::size_t total_sensor_blocks() const noexcept = 0;

    /**
     * @brief Acquires a block of sensor elements for rendering. May be not thread safe.
     * @param block_id a block index between 0 and total_sensor_blocks()
     */
    [[nodiscard]] virtual block_handle_t acquire_sensor_block(
            const film_storage_handle_t* storage,
            std::size_t block_id) const noexcept = 0;
    /**
     * @brief Releases a block post rendering. May be not thread safe.
     */
    virtual void release_sensor_block(const film_storage_handle_t* storage,
                                      block_handle_t&& block) const noexcept = 0;

    /**
     * @brief Splats an integrator sample onto the film storage from a thread pool worker. Used for direct sensor sampling techniques. storage must be created with sensor_flags_e::requires_direct_splats.
     * (thread safe, when accessed for a thread pool worker)
     * @param storage_ptr   film storage to splat to
     * @param element       sensor element
     * @param sample        integrator sample
     */
    virtual void splat_direct(film_storage_handle_t* storage_ptr,
                             const sensor_element_sample_t& element,
                             const radiant_flux_stokes_t& sample,
                             const wavenumber_t k) const noexcept = 0;
    
    /**
     * @brief Splats an integrator sample onto an image block.
     * (splatting is not thread safe)
     * @param block_handle  image block
     * @param element       sensor element
     * @param sample        integrator sample
     */
    virtual void splat(const block_handle_t& block_handle,
                       const sensor_element_sample_t& element,
                       const radiant_flux_stokes_t& sample,
                       const wavenumber_t k) const noexcept = 0;

    [[nodiscard]] virtual bool is_delta_position() const noexcept = 0;
    [[nodiscard]] virtual bool is_delta_direction() const noexcept = 0;

    /**
     * @brief Samples a time-reversed beam ("importance") around the specified film position.
     * @param element sensor element to sample from
     * @param k wavenumber
     */
    [[nodiscard]] virtual sensor_sample_t sample(sampler::sampler_t& sampler,
                                                 const vec3u32_t& element, 
                                                 const wavenumber_t k) const noexcept = 0;

    /**
     * @brief Samples a direct connection to a world position
     * @param wp world position
     * @param k wavenumber
     */
    [[nodiscard]] virtual sensor_direct_sample_t sample_direct(sampler::sampler_t& sampler,
                                                               const pqvec3_t& wp,
                                                               const wavenumber_t k) const noexcept = 0;
    
    /**
     * @brief Sampling PDF of a sensor position
     * @param p position
     */
    [[nodiscard]] virtual area_sampling_pd_t pdf_position(const pqvec3_t& p) const noexcept = 0;
    /**
     * @brief Sampling PDF of a direction
     * @param p sensor position
     * @param dir direction
     */
    [[nodiscard]] virtual solid_angle_sampling_pd_t pdf_direction(const pqvec3_t& p, const dir3_t& dir) const noexcept = 0;

public:
    static std::shared_ptr<sensor_t> load(std::string id, 
                                          scene::loader::loader_t* loader, 
                                          const scene::loader::node_t& node, 
                                          const wt::wt_context_t &context);
};

}
}
