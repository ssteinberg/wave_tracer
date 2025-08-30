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

namespace wt {

class scene_t;
namespace emitter { class emitter_t; }
namespace sensor  { class sensor_t; }

/**
 * @brief Emitter sample for a direct connection strategy.
 */
struct emitter_direct_sample_t {
    /** @brief Sampled emitter.
     */
    const emitter::emitter_t *emitter=nullptr;
    /** @brief Sampling probability mass of sampled emitter.
     *         This field is only populated by the scene-wide direct sampler `wt::scene_t::sample_emitter_direct()`, and not by the individual `wt::emitter::emitter_t::sample_direct()` of emitters.
     */
    f_t emitter_pdf=0;

    /** @brief Density of direct connection.
     */
    solid_angle_sampling_pd_t dpd;

    spectral_radiance_beam_t beam;

    std::optional<intersection_surface_t> surface;
};

struct emitter_direct_sample_pdf_t {
    /** @brief Sampling probability mass of sampled emitter.
     */
    f_t emitter_pdf=0;
    /** @brief Density of direct connection.
     */
    solid_angle_sampling_pd_t dpd;
};

/**
 * @brief Emitter sample.
 */
struct emitter_sample_t {
    spectral_radiant_flux_beam_t beam;

    /** @brief Position density.
     */
    area_sampling_pd_t ppd;
    /** @brief Direction density.
     */
    solid_angle_sampling_pd_t dpd;

    std::optional<intersection_surface_t> surface;
};

/**
 * @brief Sampled wavenumber.
 */
struct wavenumber_sample_t {
    /** @brief Sampled wavenumber.
     */
    wavenumber_t k;
    /** @brief Pdf of sampling the wavenumber.
     */
    wavenumber_sampling_pd_t wpd;

    [[nodiscard]] inline wavelength_t to_wavelength() const noexcept {
        return wavenum_to_wavelen(k);
    }
    /**
     * @brief Transforms the wavenumber density to wavelength density.
     */
    [[nodiscard]] inline wavelength_sampling_pd_t wpd_to_wavelength_density() const noexcept {
        if (wpd.is_discrete())
            return wavelength_sampling_pd_t::discrete(wpd.mass());

        const auto lambda = to_wavelength();
        return k / lambda * wpd.density();
    }
};


/**
 * @brief Sampled emitter and wavenumber, for a given sensor.
 */
struct emitter_wavenumber_sample_t {
    /** @brief Sampled emitter.
     */
    const emitter::emitter_t *emitter=nullptr;
    /** @brief Sampling probability mass of sampled emitter.
     */
    f_t emitter_pdf;

    wavenumber_sample_t wavenumber;
};
/**
 * @brief Sampled emitter and wavenumber, for a given sensor.
 */
struct emitter_beam_wavenumber_sample_t {
    /** @brief Sampled emitter.
     */
    const emitter::emitter_t *emitter=nullptr;
    /** @brief Sampling probability mass of sampled emitter.
     */
    f_t emitter_pdf;

    emitter_sample_t emitter_sample;
    wavenumber_sample_t wavenumber;
};

}
