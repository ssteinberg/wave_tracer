/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/ads/common.hpp>

#include <wt/math/common.hpp>
#include <wt/math/distribution/gaussian2d.hpp>

namespace wt::beam {

/**
 * @brief Models the wavefront of a Gaussian beam
 */
class gaussian_wavefront_t {
public:
    // beam envelope is 3*stddev of intensity distribution
    static constexpr f_t beam_cross_section_envelope = 3;

private:
    gaussian2d_t dist;
    // TODO: get rid of scale; work with distribution with proper units
    static constexpr auto scale = f_t(1)*u::m;

public:
    gaussian_wavefront_t(gaussian2d_t intensity_distribution)
        : dist(intensity_distribution)
    {
        // if the wavefront has degenerated into a flat (1D) or singular distribution (dirac)
        // revert to ray
        if (dist.is_dirac()) dist = gaussian2d_t{ { 0,0 } };
    }

    /**
     * @brief Is the beam a singular ray?
     */
    [[nodiscard]] inline bool is_ray() const noexcept { return dist.is_dirac(); }

    /**
     * @brief The cross-sectional distribution of beam intensity
     */
    [[nodiscard]] inline const auto& intensity_distribution() const noexcept { return dist; }

    /**
     * @brief The cross-sectional distribution of beam amplitude
     */
    [[nodiscard]] inline auto amplitude_magnitude_distribution() const noexcept {
        return dist.scaled(1, m::sqrt_two);
    }

    /**
     * @brief Beam intensity at a point on the beam cross section
     */
    [[nodiscard]] inline auto I(pqvec2_t x) const noexcept {
        return dist.pdf(vec2_t{ x/scale });
    }

    /**
     * @brief Beam amplitude at a point on the beam cross section
     */
    [[nodiscard]] inline auto amplitude_magnitude(pqvec2_t x) const noexcept {
        return m::sqrt(dist.pdf(vec2_t{ x/scale }));
    }

    /**
     * @brief Beam's envelope in local frame
     */
    [[nodiscard]] inline pqvec2_t envelope() const noexcept {
        return dist.std_dev() * beam_cross_section_envelope * scale;
    }

    /**
     * @brief Beam's envelope in local frame
     */
    [[nodiscard]] inline Area auto cross_section_area() const noexcept {
        return m::pi * 
               (dist.std_dev().x * scale * beam_cross_section_envelope) * 
               (dist.std_dev().y * scale * beam_cross_section_envelope);
    }

    /**
     * @brief Checks if a point is inside the beam's envelope
     */
    [[nodiscard]] inline auto is_in_envelope(pqvec2_t p) const noexcept {
        return m::length2(dist.to_canonical(vec2_t{ p/scale })) <= beam_cross_section_envelope;
    }

    /**
     * @brief Sample a point on the beam' cross section (w.r.t. intensity distribution).
     */
    [[nodiscard]] inline pqvec2_t sample(sampler::sampler_t& sampler) const noexcept {
        // rejection sample the beam cross section
        pqvec2_t sample;
        do {
            sample = dist.sample(sampler).pt * scale;
        } while(!is_in_envelope(sample));
        
        return sample;
    }

    /**
     * @brief Integrates the wavefront over the support of a triangle defined via its 3 2D vertices.
     *        Returns the radiant flux: integrated intensity.
     */
    [[nodiscard]] auto integrate_triangle(pqvec2_t pa, pqvec2_t pb, pqvec2_t pc) const noexcept {
        const auto a = vec2_t{ pa/scale };
        const auto b = vec2_t{ pb/scale };
        const auto c = vec2_t{ pc/scale };

        return dist.integrate_triangle(a,b,c);
    }
};

}
