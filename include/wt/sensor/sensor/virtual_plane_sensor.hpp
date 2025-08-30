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
#include <optional>

#include <wt/sensor/sensor/film_backed_sensor.hpp>
#include <wt/sensor/sensor/virtual_sensor.hpp>
#include <wt/sensor/film/film.hpp>
#include <wt/sensor/sensor_sample.hpp>

#include <wt/beam/beam.hpp>
#include <wt/sampler/sampler.hpp>
#include <wt/spectrum/spectrum.hpp>

#include <wt/math/common.hpp>
#include <wt/math/frame.hpp>
#include <wt/math/transform/transform.hpp>
#include <wt/math/shapes/ray.hpp>

#include <wt/wt_context.hpp>

namespace wt {

class scene_t;

namespace sensor {

/**
 * @brief A two-dimensional virtual plane sensor.
 *        Sensor consists of a virtual plane positioned in space, and the energy that falls upon its front face (as defined by its normal) is recorded on the underlying film's elements.
 *        Useful for signal coverage simulations.
 */
class virtual_plane_sensor_t final : public film_backed_sensor_scalar_t<2>, public virtual_coverage_sensor_t {
    friend class wt::scene_t;

    // the spatial standard deviation of a sourced beam, w.r.t. sensor element size
    static constexpr f_t beam_source_spatial_stddev = .25;

private:
    frame_t sensor_frame;
    pqvec3_t sensor_origin;
    pqvec2_t sensor_extent, sensor_element_extent;

    qvec2<length_density_t> recp_sensor_element_extent;
    area_density_t recp_area;

    std::optional<f_t> requested_tan_alpha;

public:
    virtual_plane_sensor_t(const wt_context_t &ctx,
                          std::string id,
                          const transform_t& transform,
                          pqvec2_t sensor_extent,
                          film_t film,
                          std::uint32_t samples_per_element,
                          bool ray_trace,
                          std::optional<f_t> requested_tan_alpha) noexcept;
    virtual_plane_sensor_t(virtual_plane_sensor_t&&) = default;
    
    [[nodiscard]] inline const auto& frame() const noexcept { return sensor_frame; }

    /**
     * @brief Returns the sensor's extent.
     */
    [[nodiscard]] inline const auto& extent() const noexcept { return sensor_extent; }

    /**
     * @brief Returns the sensor's area.
     */
    [[nodiscard]] inline auto area() const noexcept { return m::prod(sensor_extent); }

    /**
     * @brief Returns a sensor element's extent.
     */
    [[nodiscard]] inline auto element_extent() const noexcept {
        return sensor_element_extent;
    }

    [[nodiscard]] inline auto centre() const noexcept {
        return sensor_origin + extent().x * frame().t + extent().y * frame().b;
    }
    
    /**
     * @brief Total number of sensor elements (e.g., pixels), per dimension. Returns 1 for unused dimensions.
     */
    [[nodiscard]] inline vec3u32_t sensor_elements() const noexcept override {
        return resolution();
    }

    /**
     * @brief Returns the world position of an element on this virtual sensor.
     * @param element sensor element id and offset
     */
    [[nodiscard]] inline pqvec3_t position_for_element(const sensor::sensor_element_sample_t& element) const noexcept override {
        const auto local = vec2_t{
                element.element.x+element.offset.x+.5,
                element.element.y+element.offset.y+.5
            } * element_extent();
        return sensor_origin + local.x*frame().t + local.y*frame().b;
    }

    /**
     * @brief Returns the film element for a world position on this virtual sensor.
     */
    [[nodiscard]] inline sensor::sensor_element_sample_t element_for_position(const pqvec3_t& wp) const noexcept override {
        const auto& sp = wp - sensor_origin;
        const auto element_fp = vec2_t{ 
                pqvec2_t{ m::dot(sp,frame().t), m::dot(sp, frame().b) } * recp_sensor_element_extent
            };
        const auto film_element = vec2u32_t{ element_fp };
        const auto element_offset = element_fp - vec2_t{ film_element } - vec2_t{ .5,.5 };

        return {
            .element = vec3u32_t{ film_element, 1 },
            .offset  = vec3_t{ element_offset, 1 },
        };
    }

    /**
     * @brief Returns the sensor physical area.
     */
    [[nodiscard]] inline auto sensor_area() const noexcept { return m::prod(extent()); }

    [[nodiscard]] bool is_delta_position() const noexcept override  { return false; }
    [[nodiscard]] bool is_delta_direction() const noexcept override { return false; }

    [[nodiscard]] inline beam::sourcing_geometry_t sourcing_geometry(const wavenumber_t k) const noexcept {
        // TODO: correct anisotropic extents
        const auto initial_spatial_extent = 
            (this->element_extent().x + this->element_extent().y)/2 *
            beam_source_spatial_stddev * beam::gaussian_wavefront_t::beam_cross_section_envelope;

        if (requested_tan_alpha)
            return beam::sourcing_geometry_t::source(initial_spatial_extent,
                                                     *requested_tan_alpha,
                                                     k);

        return
            beam::sourcing_geometry_t::source_mub_from(
                initial_spatial_extent,
                k
            );
    }
    
    /**
     * @brief Returns the beam phase-space extent for sourced beams from this sensor for a given wavenumber
     */
    [[nodiscard]] inline beam::phase_space_extent_t sourcing_beam_extent(const wavenumber_t k) const noexcept override {
        return sourcing_geometry(k).phase_space_extent();
    }

    /**
     * @brief Sensor importance
     */
    [[nodiscard]] inline QE_t importance() const noexcept {
        // total importance flux is unity
        // spectral sensitivity is ignored here, the sensor response function is applied when splatting to film
        const QE_flux_t W = f_t(1) * u::ang::sr * square(u::m);
        return QE_t{ W / (m::pi * u::ang::sr) * recp_area };
    }

    [[nodiscard]] importance_beam_t Se(const ray_t& r, const wavenumber_t k) const noexcept {
        const auto W  = importance();
        const auto dn = m::max<f_t>(0,m::dot(r.d, frame().n));
        return {
            r,
            W * dn,     // non-polarimetric sensitivity
            k,
            this->sourcing_geometry(k)
        };
    }

    /**
     * @brief Samples a time-reversed beam ("importance") around the specified film position.
     * @param element sensor pixel to sample from
     * @param k wavenumber
     */
    [[nodiscard]] sensor_sample_t sample(sampler::sampler_t& sampler,
                                         const vec3u32_t& sensor_element,
                                         const wavenumber_t k) const noexcept override;

    /**
     * @brief Samples a direct connection to a world position
     * @param wp world position
     * @param k wavenumber
     */
    [[nodiscard]] sensor_direct_sample_t sample_direct(sampler::sampler_t& sampler,
                                                       const pqvec3_t& wp,
                                                       const wavenumber_t k) const noexcept override;

    /**
     * @brief Makes a direct connection to an incident beam
     * @param beam incident beam
     * @param range range from beam's origin over which a connection may be made
     */
    [[nodiscard]] std::optional<sensor_direct_connection_t> Si(
            const spectral_radiant_flux_beam_t& beam,
            const pqrange_t<>& range) const noexcept override;

    /**
     * @brief Sampling PDF of a sensor position
     * @param p position
     */
    [[nodiscard]] area_sampling_pd_t pdf_position(const pqvec3_t& p) const noexcept override;
    /**
     * @brief Sampling PDF of a direction
     * @param p sensor position
     * @param dir direction
     */
    [[nodiscard]] solid_angle_sampling_pd_t pdf_direction(const pqvec3_t& p, const dir3_t& dir) const noexcept override;

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::shared_ptr<virtual_plane_sensor_t> load(
            std::string id,
            scene::loader::loader_t* loader,
            const scene::loader::node_t& node,
            const wt::wt_context_t &context);
};

}
}
