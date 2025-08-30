/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <memory>

#include <wt/sensor/sensor.hpp>
#include <wt/emitter/emitter.hpp>
#include <wt/scene/emitter_sample.hpp>

#include <wt/math/common.hpp>
#include <wt/math/distribution/distribution1d.hpp>
#include <wt/math/distribution/discrete_distribution.hpp>

namespace wt::scene {

/**
 * @brief A helper wrapper around a `wt::sensor::sensor_t`. Provides emitter sampling machinery.
 */
class scene_sensor_t {
    friend class wt::scene_t;

private:
    struct emitter_sampling_data_t {
        using integrated_spectrum_distribution_t = distribution1d_t;

        /**
         * @brief Spectrum products of emitters' spectra times the sensor's sensitivity spectrum.
         *        Useful for classifying and sampling emitters for a sensor.
         */
        std::vector<std::unique_ptr<integrated_spectrum_distribution_t>> emitter_sensor_spectra;

        /**
         * @brief Distribution of powers contained in the above integrated spectra, useful for importance sampling emitters.
         */
        discrete_distribution_t<radiant_flux_t> emitters_power_distribution;

        /**
         * @brief Samples an emitter w.r.t. to the integrated spectrum of the emitters' emissions spectra over this sensor's sensitivity spectrum.
         */
        [[nodiscard]] const emitter::emitter_t* sample(const scene_t& parent, sampler::sampler_t& sampler) const noexcept;
        /**
        * @brief Sampling density of an emitter w.r.t. this sensor.
        */
        [[nodiscard]] inline f_t pdf(const emitter::emitter_t* emitter) const noexcept {
            return emitters_power_distribution.pdf(emitter->scene_emitter_idx);
        }

        /**
         * @brief Samples a wavenumber from the spectrum product of emission times the sensor's sensitivity.
         */
        [[nodiscard]] inline integrated_spectrum_distribution_t::sample_ret_t sample_wavenumber(
                sampler::sampler_t& sampler,
                const emitter::emitter_t* emitter) const noexcept {
            const auto* spectrum = emitter_sensor_spectra[emitter->scene_emitter_idx].get();
            assert(spectrum);
            if (!spectrum)
                    return {};
            return spectrum->sample(sampler);
        }
        /**
         * @brief Sampling density of wavenumber for given emitter.
         */
        [[nodiscard]] inline wavenumber_density_t pdf_wavenumber(
                const emitter::emitter_t* emitter, wavenumber_t k) const noexcept {
            const auto* spectrum = emitter_sensor_spectra[emitter->scene_emitter_idx].get();
            assert(spectrum);
            if (!spectrum)
                    return {};
            return spectrum->pdf(u::to_inv_mm(k)) * u::mm;
        }

        [[nodiscard]] static emitter_sampling_data_t build_sampling_data(
                const wt_context_t& ctx,
                const sensor::sensor_t* sensor,
                const scene_t* scene);
    };

private:
    std::shared_ptr<sensor::sensor_t> sensor;
    emitter_sampling_data_t emitter_sampler;

public:
    scene_sensor_t(const wt_context_t& ctx,
                   std::shared_ptr<sensor::sensor_t> sensor,
                   const scene_t* scene);

private:
    /**
     * @brief Given a sensor, samples an emitter from all scene emitters.
     */
    [[nodiscard]] inline const emitter::emitter_t* sample_emitter(
            const scene_t& parent,
            sampler::sampler_t& sampler) const noexcept {
        return emitter_sampler.sample(parent, sampler);
    }

    /**
     * @brief Given a sensor, samples an emitter from all scene emitters, as well as a wavenumber from the sampled emitter's spectrum (integrated over the sensor's spectrum).
     */
    [[nodiscard]] emitter_wavenumber_sample_t sample_emitter_and_spectrum(
            const scene_t& parent,
            sampler::sampler_t& sampler) const noexcept;

    /**
     * @brief Computes the spectral probability density for the given wavenumber ``k`` summed over all scene emitters: 
     */
    [[nodiscard]] inline wavenumber_density_t sum_spectral_pdf_for_all_emitters(
            const scene_t& parent,
            const std::vector<std::shared_ptr<emitter::emitter_t>>& scene_emitters,
            const wavenumber_t k) const noexcept {
        auto sum_wpds = wavenumber_density_t::zero();
        for (const auto& em : scene_emitters)
            sum_wpds += pdf_emitter(parent, em.get()) * pdf_spectral_sample(parent, em.get(), k);
        return sum_wpds;
    }

    /**
     * @brief Probability mass of sampling the emitter.
     */
    [[nodiscard]] inline f_t pdf_emitter(
            const scene_t& parent, 
            const emitter::emitter_t* emitter) const noexcept {
        return emitter_sampler.pdf(emitter);
    }

    /**
     * @brief Probability density of a wavenumber sample, given an emitter and a sensor.
     */
    [[nodiscard]] inline wavenumber_density_t pdf_spectral_sample(
            const scene_t& parent,
            const emitter::emitter_t* emitter,
            const wavenumber_t k) const noexcept {
        return emitter_sampler.pdf_wavenumber(emitter, k);
    }

public:
    [[nodiscard]] const auto* get_sensor() const noexcept { return sensor.get(); }

    [[nodiscard]] bool operator<(const scene_sensor_t& o) const noexcept { return sensor.get()<o.sensor.get(); }
};

}
