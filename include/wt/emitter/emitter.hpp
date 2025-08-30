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

#include <wt/sampler/measure.hpp>

#include <wt/sampler/sampler.hpp>
#include <wt/scene/element/scene_element.hpp>
#include <wt/scene/emitter_sample.hpp>
#include <wt/scene/position_sample.hpp>
#include <wt/interaction/polarimetric/stokes.hpp>
#include <wt/spectrum/spectrum.hpp>

#include <wt/beam/beam.hpp>

#include <wt/math/common.hpp>
#include <wt/math/shapes/ray.hpp>
#include <wt/wt_context.hpp>

namespace wt {
class scene_t;
namespace scene { class scene_sensor_t; }
}

namespace wt::emitter {

class emitter_t : public scene::scene_element_t {
    friend class wt::scene_t;
    friend class wt::scene::scene_sensor_t;

public:
    static constexpr std::string scene_element_class() noexcept { return "emitter"; }

private:
    std::size_t scene_emitter_idx;
    f_t emitter_phase_space_extent_scale = 1;

public:
    emitter_t(std::string id, f_t emitter_phase_space_extent_scale = 1)
        : scene_element_t(std::move(id)),
          emitter_phase_space_extent_scale(emitter_phase_space_extent_scale)
    {}
    emitter_t(const emitter_t&) = default;
    emitter_t(emitter_t&&) = default;
    virtual ~emitter_t() noexcept = default;
    
    [[nodiscard]] virtual bool is_area_emitter() const noexcept     { return false; }
    [[nodiscard]] virtual bool is_infinite_emitter() const noexcept { return false; }
    
    [[nodiscard]] virtual bool is_delta_position() const noexcept = 0;
    [[nodiscard]] virtual bool is_delta_direction() const noexcept = 0;

    /**
     * @brief Returns the requested scale factor for the phase-space extent of emitted radiation beams.
     */
    [[nodiscard]] auto get_requested_phase_space_extent_scale() const noexcept { return emitter_phase_space_extent_scale; }

    /**
     * @brief Returns the emitter's emission spectrum
     */
    [[nodiscard]] virtual const spectrum::spectrum_real_t& emission_spectrum() const noexcept = 0;

    /**
     * @brief Computes total emitted spectral power.
     */
    [[nodiscard]] virtual spectral_radiant_flux_t power(wavenumber_t k) const noexcept = 0;
    /**
     * @brief Computes total emitted power over a wavenumber range.
     */
    [[nodiscard]] virtual radiant_flux_t power(const range_t<wavenumber_t>& krange) const noexcept = 0;

    /**
     * @brief Integrate a detector beam over the emitter.
     * @param beam detection beam incident upon the emitter
     */
    [[nodiscard]] virtual spectral_radiant_flux_stokes_t Li(
            const importance_flux_beam_t& beam,
            const intersection_surface_t* surface) const noexcept = 0;

    /**
     * @brief Samples an emission phase-space position (ray) on the light source
     * @param k wavenumber
     */
    [[nodiscard]] virtual emitter_sample_t sample(sampler::sampler_t& sampler,
                                                  const wavenumber_t k) const noexcept = 0;

    /**
     * @brief Samples an emission phase-space position (ray) on the light source
     */
    [[nodiscard]] virtual position_sample_t sample_position(sampler::sampler_t& sampler) const noexcept = 0;

    /**
     * @brief Samples a direct connection to a world position
     * @param wp world position
     * @param k wavenumber
     */
    [[nodiscard]] virtual emitter_direct_sample_t sample_direct(
            sampler::sampler_t& sampler,
            const pqvec3_t& wp,
            const wavenumber_t k) const noexcept = 0;

    /**
     * @brief Sampling PDF of an emission position on the light source
     * @param p position on light source
     * @param surface emitter surface intersection record (this is required for area emitters)
     */
    [[nodiscard]] virtual area_sampling_pd_t pdf_position(
            const pqvec3_t& p,
            const intersection_surface_t* surface=nullptr) const noexcept = 0;
    /**
     * @brief Sampling PDF of an emission direction from the light source
     * @param p position on light source
     * @param dir emission direction
     * @param surface emitter surface intersection record (this is required for area emitters)
     */
    [[nodiscard]] virtual solid_angle_sampling_pd_t pdf_direction(
            const pqvec3_t& p,
            const dir3_t& dir, 
            const intersection_surface_t* surface=nullptr) const noexcept = 0;
    /**
     * @brief Sampling PDF of an emission position on the light source
     * @param surface emitter surface intersection record
     */
    [[nodiscard]] area_sampling_pd_t pdf_position(const intersection_surface_t& surface) const noexcept {
        return pdf_position(surface.wp, &surface);
    }
    /**
     * @brief Sampling PDF of an emission direction from the light source
     * @param surface emitter surface intersection record
     * @param dir emission direction
     */
    [[nodiscard]] solid_angle_sampling_pd_t pdf_direction(
            const intersection_surface_t& surface,
            const dir3_t& dir) const noexcept {
        return pdf_direction(surface.wp, dir, &surface);
    }

    /**
     * @brief Sampling PDF of a direct connection.
     * @param wp world position from which direct sampling was applied
     * @param sampled_r sampled emitter phase-space position (ray)
     * @param sampled_surface sampled surface for emitter samples that seat on a surface (required for area emitters)
     * @return the solid angle density from ``wp``
     */
    [[nodiscard]] virtual solid_angle_sampling_pd_t pdf_direct(
            const pqvec3_t& wp,
            const ray_t& r,
            const intersection_surface_t* surface=nullptr) const noexcept = 0;
    /**
     * @brief Sampling PDF of a direct connection.
     * @param wp world position from which direct sampling was applied
     * @param sampled_r sampled emitter phase-space position (ray)
     * @param sampled_surface sampled surface for emitter samples that seat on a surface (required for area emitters)
     * @return the solid angle density from ``wp``
     */
    [[nodiscard]] inline solid_angle_sampling_pd_t pdf_direct(
            const intersection_surface_t& surface,
            const ray_t& r) const noexcept {
        return pdf_direct(surface.wp, r, &surface);
    }

public:
    static std::unique_ptr<emitter_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);

    [[nodiscard]] virtual scene::element::info_t description() const override = 0;
};

}

