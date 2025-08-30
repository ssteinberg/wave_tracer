/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cassert>

#include <wt/ads/common.hpp>
#include <wt/ads/intersection_record.hpp>

#include <wt/interaction/intersection.hpp>
#include <wt/beam/gaussian_wavefront.hpp>

#include <wt/scene/shape.hpp>

#include <wt/math/common.hpp>
#include <wt/math/frame.hpp>
#include <wt/math/shapes/elliptic_cone.hpp>
#include <wt/math/distribution/gaussian2d.hpp>

#include <wt/interaction/polarimetric/stokes.hpp>
#include <wt/interaction/polarimetric/mueller.hpp>
#include <wt/bsdf/bsdf.hpp>

#include <wt/beam/beam_geometry.hpp>

namespace wt::beam {

/**
 * @brief Handles the geometric aspects of beams.
 */
class beam_generic_t {
protected:
    // geometric envelope of beam
    elliptic_cone_t envelope;

    // initial ballistic length
    length_t self_intersection_distance;

public:
    /**
     * @brief Returns the scaling factor for the beam footprint along the z-axis (direction of propagation) as a function of the major axis (x-axis).
     */
    static inline constexpr f_t major_axis_to_z_scale() noexcept { return 2; }

protected:
    beam_generic_t(const ray_t& ray,
                   const sourcing_geometry_t& sourcing_geometry) noexcept
        : envelope(sourcing_geometry.envelope(ray, self_intersection_distance))
    {
        assert(self_intersection_distance>=0*u::m);
    }
    beam_generic_t(const elliptic_cone_t& envelope) noexcept
        : envelope(envelope)
    {
        self_intersection_distance = 0*u::m;
    }

public:
    beam_generic_t(const beam_generic_t&) noexcept = default;
    beam_generic_t& operator=(const beam_generic_t&) noexcept = default;

    virtual ~beam_generic_t() noexcept = default;

    /**
     * @brief Returns the beam's wavenumber.
     */
    [[nodiscard]] virtual wavenumber_t k() const noexcept = 0;
    /**
     * @brief Returns the beam's wavelength.
     */
    [[nodiscard]] auto wavelength() const noexcept { return wavenum_to_wavelen(k()); }

    /**
     * @brief Returns beam's envelope.
     */
    [[nodiscard]] inline const auto& get_envelope() const noexcept { return envelope; }

    /**
     * @brief Beam mean direction.
     */
    [[nodiscard]] inline const auto& dir() const noexcept { return envelope.d(); }
    /**
     * @brief Beam origin.
     */
    [[nodiscard]] inline const auto& origin() const noexcept { return envelope.o(); }
    /**
     * @brief Returns TRUE if beam originates from infinity.
     */
    [[nodiscard]] inline bool from_infinity() const noexcept { return !m::isfinite(envelope.o()); }

    /**
     * @brief Beam mean ray.
     */
    [[nodiscard]] inline const auto& mean_ray() const noexcept { return envelope.ray(); }

    /**
     * @brief Beam's local frame.
     */
    [[nodiscard]] inline auto frame() const noexcept {
        return envelope.frame();
    }

    /**
     * @brief Beam' spatial three-dimensional footprint at propagation distance dist
     *        (in beam's local frame)
     */
    [[nodiscard]] inline pqvec3_t footprint(length_t dist) const noexcept {
        const auto a = envelope.axes(dist);
        return pqvec3_t{ a.x, a.y, major_axis_to_z_scale()*a.x };
    }

    /**
     * @brief Beam' spatial standard deviation over local x,y,z axes at propagation distance dist.
     *        (in beam's local frame)
     */
    [[nodiscard]] inline pqvec3_t std_dev(length_t dist) const noexcept {
        return footprint(dist) / beam_cross_section_envelope;
    }

    /**
     * @brief Beam' wavefront, in local frame, at propagation distance dist.
     */
    [[nodiscard]] inline gaussian_wavefront_t wavefront(length_t dist) const noexcept {
        const auto& sigmas = std_dev(dist);
        const auto scale = 1*u::m;
        return {
            gaussian2d_t{ 
                vec2_t{ pqvec2_t{ sigmas.x,sigmas.y } / scale }, 
                { 1,0 }
            }
        };
    }

    /**
     * @brief Returns the point 'p' after projection onto the beam cross section after propagation a distance of 'beam_dist'.
     */
    [[nodiscard]] inline pqvec2_t project(const pqvec3_t& p, length_t beam_dist) const noexcept {
        return envelope.project(p, beam_dist);
    }

    /**
     * @brief Returns true if this beam is a ray.
     */
    [[nodiscard]] inline bool is_ray() const noexcept {
        return envelope.is_ray();
    }

    /**
     * @brief Returns the footprint on intersection with a surface
     *
     * @param surface surface intersection
     * @param beam_z_dist beam propagation distance
     */
    [[nodiscard]] intersection_footprint_t surface_footprint_ellipsoid(
            const intersection_surface_t& surface, 
            const length_t beam_z_dist) const noexcept;

    /**
     * @brief Returns the footprint on intersection with a surface
     *
     * @param surface surface intersection
     * @param beam_z_dist beam propagation distance
     */
    [[nodiscard]] intersection_footprint_t surface_footprint_static(
            const intersection_surface_t& surface,
            const length_t beam_z_dist) const noexcept {
        const auto ls = footprint(beam_z_dist);
        const auto x = surface.geo.to_local(envelope.x());

        assert(ls.x>0*u::m && ls.y>0*u::m);

        if (x.x!=0||x.y!=0) {
            return {
                .x =  m::normalize(vec2_t{ x.x,x.y }),
                .la = ls.x,
                .lb = ls.y,
            };
        } else {
            const auto avg = (ls.x+ls.y)/f_t(2);
            return {
                .x =  dir2_t{ 1,0 },
                .la = avg,
                .lb = avg,
            };
        }
    }
};


template <typename T>
concept Beam = std::derived_from<T, beam_generic_t>;

}
