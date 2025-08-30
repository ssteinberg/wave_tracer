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

#include <wt/spectrum/spectrum.hpp>
#include <wt/emitter/emitter.hpp>

#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

namespace wt::emitter {

/**
 * @brief An idealised, simple isotropic point emitter.
 */
class point_t final : public emitter_t {
private:
    pqvec3_t position;
    std::shared_ptr<spectrum::spectrum_real_t> spectrum;

    std::optional<length_t> extent;

public:
    point_t(std::string id, 
            const pqvec3_t& position, 
            std::shared_ptr<spectrum::spectrum_real_t> radiant_intensity,
            std::optional<length_t> extent,
            f_t emitter_phase_space_extent_scale = 1)
        : emitter_t(std::move(id), emitter_phase_space_extent_scale), 
          position(position),
          spectrum(std::move(radiant_intensity)),
          extent(extent)
    {}
    point_t(const point_t&) = default;
    point_t(point_t&&) = default;
    
    [[nodiscard]] inline bool is_delta_position() const noexcept override { return true; }
    [[nodiscard]] inline bool is_delta_direction() const noexcept override { return false; }

    [[nodiscard]] inline spectral_radiant_intensity_t spectral_radiant_intensity(const wavenumber_t k) const noexcept {
        return spectrum->f(k) * spectral_radiant_intensity_t::unit;
    }
    
    /**
     * @brief Returns the emitter's emission spectrum
     */
    [[nodiscard]] const spectrum::spectrum_real_t& emission_spectrum() const noexcept override {
        return *spectrum;
    }
    
    /**
     * @brief Computes total emitted spectral power.
     */
    [[nodiscard]] inline spectral_radiant_flux_t power(wavenumber_t k) const noexcept override {
        return spectral_radiant_intensity(k) * (m::four_pi * u::ang::sr);
    }
    /**
     * @brief Computes total emitted power over a wavenumber range.
     */
    [[nodiscard]] inline radiant_flux_t power(const range_t<wavenumber_t>& krange) const noexcept override {
        return spectrum->power(krange) * (u::W / u::ang::sr) * 
               (m::four_pi * u::ang::sr);
    }

    [[nodiscard]] inline beam::sourcing_geometry_t sourcing_geometry(const wavenumber_t k) const noexcept {
        // point sources are not physical, default to a fake a spatial extent of 5λ
        static constexpr f_t lambda_to_extent = 10;
        const auto initial_spatial_extent =
            extent.value_or(lambda_to_extent * wavenum_to_wavelen(k));

        const auto se =
            beam::sourcing_geometry_t::source_mub_from(
                initial_spatial_extent,
                k
            ).phase_space_extent().enlarge(get_requested_phase_space_extent_scale());
        return
            beam::sourcing_geometry_t::source(se);
    }

    /**
     * @brief Source a beam from this light source.
     * @param k wavenumber
     */
    [[nodiscard]] spectral_radiant_intensity_beam_t Le(
            const ray_t& r, 
            const wavenumber_t k) const noexcept {
        return spectral_radiant_intensity_beam_t{
            r, spectral_radiant_intensity(k),
            k,
            sourcing_geometry(k)
        };
    }

    /**
     * @brief Integrate a detector beam over the emitter.
     * @param beam detection beam incident upon the emitter
     */
    [[nodiscard]] spectral_radiant_flux_stokes_t Li(
            const importance_flux_beam_t& beam,
            const intersection_surface_t* surface=nullptr) const noexcept override {
        // delta position emitter: well designed integrators should not arrive here
        assert(false);
        return {};
    }

    /**
     * @brief Samples an emission phase-space position (ray) on the light source
     * @param k wavenumber
     */
    [[nodiscard]] emitter_sample_t sample(sampler::sampler_t& sampler,
                                          const wavenumber_t k) const noexcept override;

    /**
     * @brief Samples an emission phase-space position (ray) on the light source
     */
    [[nodiscard]] position_sample_t sample_position(sampler::sampler_t& sampler) const noexcept override {
        return position_sample_t{
            .p = position,
            .ppd = area_sampling_pd_t::discrete(1),
        };
    }

    /**
     * @brief Samples a direct connection to a world position
     * @param wp world position
     * @param k wavenumber
     */
    [[nodiscard]] emitter_direct_sample_t sample_direct(
            sampler::sampler_t& sampler,
            const pqvec3_t& wp,
            const wavenumber_t k) const noexcept override;

    /**
     * @brief Sampling PDF of an emission position on the light source
     * @param p position on light source
     * @param surface emitter surface intersection record (should be ``nullptr``)
     */
    [[nodiscard]] area_sampling_pd_t pdf_position(
            const pqvec3_t& p,
            const intersection_surface_t* surface=nullptr) const noexcept override {
        return area_sampling_pd_t::discrete(1);
    }

    /**
     * @brief Sampling PDF of an emission direction from the light source
     * @param p position on light source
     * @param dir emission direction
     * @param surface emitter surface intersection record (should be ``nullptr``)
     */
    [[nodiscard]] solid_angle_sampling_pd_t pdf_direction(
            const pqvec3_t& p,
            const dir3_t& dir, 
            const intersection_surface_t* surface=nullptr) const noexcept override {
        return solid_angle_density_t{ sampler::sampler_t::uniform_sphere_pdf() / u::ang::sr };
    }

    /**
     * @brief Sampling PDF of a direct connection.
     * @param wp world position from which direct sampling was applied
     * @param sampled_r sampled emitter phase-space position (ray)
     * @param sampled_surface sampled surface for emitter samples that seat on a surface (should be ``nullptr``)
     * @return the solid angle density from ``wp``
     */
    [[nodiscard]] solid_angle_sampling_pd_t pdf_direct(
            const pqvec3_t& wp,
            const ray_t& r,
            const intersection_surface_t* surface=nullptr) const noexcept override {
        return solid_angle_sampling_pd_t::discrete(1);
    }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::unique_ptr<emitter_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);
};

}

