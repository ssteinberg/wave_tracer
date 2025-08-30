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
#include <wt/interaction/surface_profile/surface_profile.hpp>
#include <wt/interaction/fresnel.hpp>

#include <wt/wt_context.hpp>

#include "bsdf.hpp"

namespace wt::bsdf {

/**
 * @brief Generic smooth to moderately-rough surface with arbitrary (real or complex) IOR and surface profile. The surface might be a transmissive dielectric or a conductor at different parts of the spectrum.
 *        Scattering is formalised via 1-st order SPM.
 */
class surface_spm_t final : public bsdf_t {
private:
    std::shared_ptr<spectrum::spectrum_t> extIOR;
    std::shared_ptr<spectrum::spectrum_t> IORn;
    std::shared_ptr<surface_profile::surface_profile_t> profile;

    std::shared_ptr<spectrum::spectrum_real_t> reflection_scale;
    std::shared_ptr<spectrum::spectrum_real_t> transmission_scale;

public:
    static constexpr int lobe_specular  = 0;
    static constexpr int lobe_scattered = 1;

public:
    surface_spm_t(std::string id, 
                  std::shared_ptr<spectrum::spectrum_t> extIOR,
                  std::shared_ptr<spectrum::spectrum_t> IORn, 
                  std::shared_ptr<surface_profile::surface_profile_t> profile,
                  std::shared_ptr<spectrum::spectrum_real_t>&& reflection_scale,
                  std::shared_ptr<spectrum::spectrum_real_t>&& transmission_scale)
        : bsdf_t(std::move(id)), 
          extIOR(std::move(extIOR)),
          IORn(std::move(IORn)),
          profile(std::move(profile)),
          reflection_scale(std::move(reflection_scale)),
          transmission_scale(std::move(transmission_scale))
    {}
    surface_spm_t(surface_spm_t&&) = default;

    [[nodiscard]] inline auto IOR(const wavenumber_t k) const noexcept {
        const auto eta_1 = extIOR->value(k);
        const auto eta_2 = IORn->value(k);
        return eta_1/eta_2;
    }

    [[nodiscard]] inline auto reflectivity_scale(const wavenumber_t& k) const noexcept {
        return reflection_scale ? reflection_scale->f(k) : 1;
    }
    [[nodiscard]] inline auto transmissivity_scale(const wavenumber_t& k) const noexcept {
        return transmission_scale ? transmission_scale->f(k) : 1;
    }

    /**
     * @brief Spectral albedo. Returns std::nullopt when albedo cannot be computed.
     * @param k wavenumber
     */
    [[nodiscard]] inline std::optional<f_t> albedo(const wavenumber_t k) const noexcept override {
        const auto ior = IOR(k);
        const auto f = fresnel(ior, { 0,0,1 });
        return (std::norm(f.rs)+std::norm(f.rp))/2 * reflectivity_scale(k);
    }
    
    /**
     * @brief Returns mask of all available lobes for this BSDF at particular wavenumber.
     */
    [[nodiscard]] lobe_mask_t lobes(wavenumber_t k) const noexcept override {
        lobe_mask_t lobes{};
        lobes.set(lobe_specular);
        if (!profile->is_delta_only(k))
            lobes.set(lobe_scattered);
        return lobes;
    }
    
    /**
     * @brief Does this BSDF comprise of only delta lobes?
     */
    [[nodiscard]] inline bool is_delta_only(wavenumber_t k) const noexcept override {
        return profile->is_delta_only(k);
    }
    
    /**
     * @brief Is a lobe a delta lobe?
     */
    [[nodiscard]] inline bool is_delta_lobe(wavenumber_t k, std::uint32_t lobe) const noexcept override {
        return lobe == lobe_specular;
    }

    /**
     * @brief Evaluates the BSDF. Accounts for the cosine foreshortening term.
     */
    [[nodiscard]] bsdf_result_t f(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept override;

    /**
     * @brief Samples the BSDF. The returned weight is bsdf/pdf. 
     * @param S normalized Stokes parameters vector that describes the polarimetric properties of incident radiation.
     */
    [[nodiscard]] std::optional<bsdf_sample_t> sample(
            const dir3_t &wi,
            const bsdf_query_t& query, 
            sampler::sampler_t& sampler) const noexcept override;
    
    /**
     * @brief Provides the sample density. 
     * @param S normalized Stokes parameters vector that describes the polarimetric properties of incident radiation.
     */
    [[nodiscard]] solid_angle_density_t pdf(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept override;

    /**
     * @brief Computes the refractive-index ratio: eta at exit / eta at entry.
     * @param k wavenumber
     */
    [[nodiscard]] f_t eta(
            const dir3_t &wi,
            const dir3_t &wo,
            const wavenumber_t k) const noexcept override {
        const auto eta_1 = std::real(extIOR->value(k));
        const auto eta_2 = std::real(IORn->value(k));

        return wi.z>=0 ? eta_1/eta_2 : eta_2/eta_1;
     }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::unique_ptr<bsdf_t> load(std::string id, scene::loader::loader_t* loader, const scene::loader::node_t& node, const wt::wt_context_t &context);
};

}
