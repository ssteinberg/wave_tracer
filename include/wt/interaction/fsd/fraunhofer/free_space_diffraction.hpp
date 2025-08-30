/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/ads/intersection_record.hpp>
#include <wt/sampler/sampler.hpp>

#include <wt/beam/gaussian_wavefront.hpp>

#include <wt/math/common.hpp>
#include <wt/math/frame.hpp>
#include <wt/math/shapes/elliptic_cone.hpp>
#include <wt/sampler/density.hpp>

#include <wt/interaction/fsd/fraunhofer/fsd.hpp>
#include <wt/interaction/fsd/fraunhofer/fsd_sampler.hpp>

namespace wt::fraunhofer {

/**
 * @brief Free-space diffraction angular scattering function (ASF / BSDF).
 *        Formalises edge-based diffraction of beams, under the Fraunhofer approximation, for arbitrary apertures.
 *        Sampling and evaluations are handled by fsd_sampler, and are done in a wavenumber-agnostic space.
 *
 *        For more information: "A Free-Space Diffraction BSDF", Steinberg et al., SIGGRAPH 2024.
 */
class free_space_diffraction_t {
private:
    static constexpr auto fsd_unit = f_t(1) * u::mm;
    static constexpr f_t  wo2_cutoff = .85;

    fsd::fsd_aperture_t aperture;

    wavenumber_t k;
    frame_t frame;

    const fsd_sampler::fsd_sampler_t *fsd_sampler;

public:
    struct sample_ret_t {
        dir3_t wo;
        solid_angle_sampling_pd_t dpd;
        f_t weight;
    };

public:
    /**
     * @brief Construct a new free space diffraction object
     * 
     * @param k wavenumber
     * @param psi0 integrated beam amplitude over opening
     * @param totalPower integrated beam power impinging upon opening
     * @param edges edges accessor
     * @param wave_function wave function Gaussian
     * @param interaction_point interaction point within the beam
     */
    free_space_diffraction_t(const ads::ads_t* ads,
                             const fsd_sampler::fsd_sampler_t *fsd_sampler,
                             const frame_t& frame,
                             wavenumber_t k, f_t totalPower,
                             const elliptic_cone_t& beam,
                             const ads::intersection_record_t::edges_container_t& edges,
                             const beam::gaussian_wavefront_t& wave_function) noexcept;
    free_space_diffraction_t(free_space_diffraction_t&&) noexcept = default;

    /**
     * @brief Returns TRUE if the aperture is empty
     */
    [[nodiscard]] bool empty() const noexcept { return aperture.edges.empty(); }

    [[nodiscard]] const auto& get_frame() const noexcept { return frame; }

    /**
     * @brief Samples the free-space diffraction BSDF. The returned weight is bsdf/pdf. 
     */
    [[nodiscard]] sample_ret_t sample(sampler::sampler_t& sampler) const noexcept {
        const auto sample = fsd_sampler->sample(sampler, aperture);
        const auto scale  = u::to_num(this->k * fsd_unit);

        if (sample.pdf>0) {
            // tan to sin and scale
            const auto zeta    = sample.xi / scale;
            const auto wolocal = zeta / (m::sqrt(vec2_t{ 1 }+m::sqr(zeta)));
            const auto wo2     = m::length2(wolocal);
            if (wo2 < wo2_cutoff) {
                const auto z = m::sqrt(1-wo2);
                return {
                    .wo = dir3_t{ wolocal,z },
                    .dpd = solid_angle_density_t{ sample.pdf / u::ang::sr },
                    .weight = sample.weight,
                };
            }
        }

        return {
            .wo = { 0,0,1 },
            .dpd = solid_angle_density_t::zero(), 
            .weight = 0,
        };
    }

    /**
     * @brief Evaluates the free-space diffraction BSDF.
     */
    [[nodiscard]] f_t f(const dir3_t &wolocal) const noexcept {
        return (f_t)(pdf(wolocal) * u::ang::sr);
    }

    /**
     * @brief Queries the sampling density of the free-space diffraction BSDF.
     *        (approximation)
     */
    [[nodiscard]] solid_angle_density_t pdf(const dir3_t& wolocal) const noexcept {
        const auto wo2 = m::length2(vec2_t{ wolocal });
        if (wolocal.z<=0 || wo2>=wo2_cutoff)
            return solid_angle_density_t::zero();

        const auto scale = u::to_num(this->k * fsd_unit);
        // sin to tan and scale
        const auto zeta = vec2_t{ wolocal } / m::sqrt(vec2_t{ 1 }-m::sqr(vec2_t{ wolocal }));
        const auto xi   = scale * zeta;

        const auto pdf = fsd_sampler->pdf(aperture, xi);
        assert(m::isfinite(pdf) && pdf>=0);

        return (0<=pdf&&pdf<f_t(1e+2) ? pdf : 0) / u::ang::sr;
    }
};

}
