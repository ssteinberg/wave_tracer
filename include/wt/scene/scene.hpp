/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <set>

#include <wt/wt_context.hpp>

#include <wt/integrator/integrator.hpp>
#include <wt/ads/ads.hpp>

#include <wt/scene/shape.hpp>
#include <wt/bsdf/bsdf.hpp>
#include <wt/sensor/sensor.hpp>

#include <wt/emitter/emitter.hpp>
#include <wt/scene/emitter_sample.hpp>

#include <wt/math/common.hpp>
#include <wt/math/shapes/aabb.hpp>

#include "scene_sensor.hpp"

namespace wt {

/**
 * @brief Holds all scene data, and provides emitter and spectral sampling facilities.
 */
class scene_t {
public:
    /**
     * @brief This defines the max count of sensors that the scene is willing to handle.
     *        WIP: machinery for multi-sensor rendering is working on scene side.
     *          However, currently multi-sensor rendering does not always makes sense,
     *          and we might want dedicated integrator support.
     */
    static constexpr std::size_t max_supported_sensors = 1;

private:
    std::string id;

    std::shared_ptr<integrator::integrator_t> scene_integrator;
    std::vector<std::shared_ptr<emitter::emitter_t>> scene_emitters;
    std::vector<std::shared_ptr<shape_t>> scene_shapes;
    std::set<scene::scene_sensor_t> scene_sensors;

    aabb_t world_aabb;

    // sampler_t is expected to be thread safe
    mutable std::shared_ptr<sampler::sampler_t> scene_sampler;

private:
    [[nodiscard]] inline const scene::scene_sensor_t* get_scene_sensor(const sensor::sensor_t* sensor) const noexcept {
        const auto it = std::ranges::lower_bound(scene_sensors, sensor, {},
                                                 [] (const auto& scs) { return scs.get_sensor(); });
        if (it==scene_sensors.cend() || it->get_sensor()!=sensor) {
            assert(false);
            return nullptr;
        }
        return &*it;
    }

public:
    scene_t(std::string id,
            const wt_context_t& ctx,
            std::shared_ptr<integrator::integrator_t> integrator,
            std::vector<std::shared_ptr<sensor::sensor_t>>&& sensors,
            std::shared_ptr<sampler::sampler_t> sampler,
            std::vector<std::shared_ptr<emitter::emitter_t>> emitters,
            std::vector<std::shared_ptr<shape_t>> shapes);

    [[nodiscard]] inline const auto& get_id() const { return id; }

    [[nodiscard]] inline const auto& integrator() const { return *scene_integrator; }
    [[nodiscard]] inline const auto& sensors() const    { return scene_sensors; }
    [[nodiscard]] inline auto& sampler() const noexcept { return *scene_sampler; }
    [[nodiscard]] inline const auto& shapes() const     { return scene_shapes; }
    [[nodiscard]] inline const auto& emitters() const   { return scene_emitters; }
    
    [[nodiscard]] inline const auto& get_world_aabb() const { return world_aabb; }

    /**
     * @brief Given a sensor, samples an emitter from all scene emitters, as well as a wavenumber from the sampled emitter's spectrum (integrated over the sensor's spectrum).
     * @param sensor used sensor
     */
    [[nodiscard]] inline emitter_wavenumber_sample_t sample_emitter_and_spectrum(
            const sensor::sensor_t* sensor) const noexcept {
        const auto* scs = get_scene_sensor(sensor);
        return scs->sample_emitter_and_spectrum(*this, sampler());
    }

    /**
     * @brief Given a sensor, samples an emitter from all scene emitters, as well as a wavenumber from the sampled emitter's spectrum (integrated over the sensor's spectrum), then sources a beam from the sampled emitter.
     *        Does NOT divide by the emitter or wavelength sampling density.
     * @param sensor used sensor
     */
    [[nodiscard]] inline emitter_beam_wavenumber_sample_t sample_emitter_and_spectrum_and_source_beam(
            const sensor::sensor_t* sensor) const noexcept {
        const auto s = this->sample_emitter_and_spectrum(sensor);
        auto emitter_sample = s.emitter->sample(sampler(), s.wavenumber.k);

        return {
            .emitter = s.emitter,
            .emitter_pdf = s.emitter_pdf,
            .emitter_sample = std::move(emitter_sample),
            .wavenumber = s.wavenumber,
        };
    }

    /**
     * @brief Samples a direct connection from a world position to a scene emitter for a given sensor.
     *        Divides by the sampled emitter's sampling probability mass. Does NOT divide by the wavelength sampling density.
     * @param sampler sampler to use (overriding the scene's sampler)
     * @param sensor used sensor
     * @param wp world position from which direct sampling is applied
     * @param k wavenumber
     */
    [[nodiscard]] inline emitter_direct_sample_t sample_emitter_direct(
            sampler::sampler_t& sampler,
            const sensor::sensor_t* sensor,
            const pqvec3_t& wp,
            const wavenumber_t k) const noexcept {
        const auto* scs = get_scene_sensor(sensor);

        const auto* emitter = scs->sample_emitter(*this, sampler);
        const auto pd = scs->pdf_emitter(*this, emitter);

        auto sample = emitter->sample_direct(sampler, wp, k);
        sample.emitter_pdf = pd;
        sample.beam /= pd;
        return sample;
    }
    /**
     * @brief Samples a direct connection from a world position to a scene emitter for a given sensor.
     *        Divides by the sampled emitter's sampling probability mass. Does NOT divide by the wavelength sampling density.
     * @param sensor used sensor
     * @param wp world position from which direct sampling is applied
     * @param k wavenumber
     */
    [[nodiscard]] inline emitter_direct_sample_t sample_emitter_direct(
            const sensor::sensor_t* sensor,
            const pqvec3_t& wp,
            const wavenumber_t k) const noexcept {
        return sample_emitter_direct(sampler(), sensor, wp, k);
    }

    /**
     * @brief Probability density of a sampled direct connection from an emitter to a world position.
     * @param sensor used sensor
     * @param emitter sampled emitter
     * @param wp world position from which direct sampling was applied
     * @param sample sampled emitter phase-space position (ray)
     * @param sampled_surface sampled surface for emitter samples that seat on a surface (required for some emitters)
     */
    [[nodiscard]] inline emitter_direct_sample_pdf_t pdf_emitter_direct(
            const sensor::sensor_t* sensor,
            const emitter::emitter_t* emitter,
            const pqvec3_t& wp,
            const ray_t& sample,
            const intersection_surface_t* sampled_surface = nullptr) const noexcept {
        const auto* scs = get_scene_sensor(sensor);
        return {
            .emitter_pdf = scs->pdf_emitter(*this, emitter),
            .dpd = emitter->pdf_direct(wp, sample, sampled_surface),
        };
    }

    /**
     * @brief Computes the spectral probability density for the given wavenumber ``k`` summed over all scene emitters: 
     *        Useful for spectral MIS.
     * @param sensor used sensor
     * @param k sampled wavenumber
     */
    [[nodiscard]] inline wavenumber_density_t sum_spectral_pdf_for_all_emitters(
            const sensor::sensor_t* sensor,
            const wavenumber_t k) const noexcept {
        const auto* scs = get_scene_sensor(sensor);
        return scs->sum_spectral_pdf_for_all_emitters(*this, this->emitters(), k);
    }

    /**
     * @brief Probability mass of sampling the emitter.
     * @param sensor used sensor
     * @param emitter sampled emitter
     */
    [[nodiscard]] inline f_t pdf_emitter(const sensor::sensor_t* sensor, 
                                         const emitter::emitter_t* emitter) const noexcept {
        const auto* scs = get_scene_sensor(sensor);
        return scs->pdf_emitter(*this, emitter);
    }

    /**
     * @brief Probability density of a wavenumber sample, given an emitter and a sensor.
     * @param sensor used sensor
     * @param emitter used emitter
     * @param k sampled wavenumber
     */
    [[nodiscard]] inline wavenumber_density_t pdf_spectral_sample(
            const sensor::sensor_t* sensor, 
            const emitter::emitter_t* emitter, 
            const wavenumber_t k) const noexcept {
        const auto* scs = get_scene_sensor(sensor);
        return scs->pdf_spectral_sample(*this, emitter, k);
    }

    /**
     * @brief Joint probability density of a wavenumber & emitter sample pair, given a sensor.
     * @param sensor used sensor
     * @param emitter sampled emitter
     * @param k wavenumber
     */
    [[nodiscard]] inline wavenumber_density_t pdf_emitter_and_spectral_sample(
            const sensor::sensor_t* sensor, 
            const emitter::emitter_t* emitter, 
            const wavenumber_t k) const noexcept {
        const auto* scs = get_scene_sensor(sensor);
        return scs->pdf_emitter(*this, emitter) *
               scs->pdf_spectral_sample(*this, emitter, k);
    }

    [[nodiscard]] scene::element::info_t description() const;
};

}
