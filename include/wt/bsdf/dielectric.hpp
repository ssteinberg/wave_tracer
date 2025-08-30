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
#include <wt/interaction/fresnel.hpp>

#include <wt/wt_context.hpp>

#include "bsdf.hpp"

namespace wt::bsdf {

/**
 * @brief Smooth dielectric interface.
 */
class dielectric_t final : public bsdf_t {
private:
    std::shared_ptr<spectrum::spectrum_t> extIOR;
    std::shared_ptr<spectrum::spectrum_t> IORn;

    std::shared_ptr<spectrum::spectrum_real_t> reflection_scale;
    std::shared_ptr<spectrum::spectrum_real_t> transmission_scale;

public:
    dielectric_t(std::string id, 
                 std::shared_ptr<spectrum::spectrum_t> extIOR,
                 std::shared_ptr<spectrum::spectrum_t> IOR,
                 std::shared_ptr<spectrum::spectrum_real_t>&& reflection_scale,
                 std::shared_ptr<spectrum::spectrum_real_t>&& transmission_scale)
        : bsdf_t(std::move(id)), 
          extIOR(std::move(extIOR)),
          IORn(std::move(IOR)),
          reflection_scale(std::move(reflection_scale)),
          transmission_scale(std::move(transmission_scale))
    {}
    dielectric_t(dielectric_t&&) = default;

    [[nodiscard]] inline auto IOR(const wavenumber_t k) const noexcept {
        const auto eta_1 = extIOR->value(k);
        const auto eta_2 = IORn->value(k);
        assert((eta_1/eta_2).imag()<1e-3);
        return (eta_1/eta_2).real();
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
        const auto f = fresnel(IOR(k), { 0,0,1 });
        return (std::norm(f.rs)+std::norm(f.rp))/2 * reflectivity_scale(k);
    }
    
    /**
     * @brief Returns mask of all available lobes for this BSDF at particular wavenumber.
     */
    [[nodiscard]] lobe_mask_t lobes(wavenumber_t k) const noexcept override {
        lobe_mask_t lobes{};
        lobes.set(0);
        return lobes;
    }
    
    /**
     * @brief Does this BSDF comprise of only delta lobes?
     */
    [[nodiscard]] inline bool is_delta_only(wavenumber_t k) const noexcept override { return true; }
    
    /**
     * @brief Is a lobe a delta lobe?
     */
    [[nodiscard]] inline bool is_delta_lobe(wavenumber_t k, std::uint32_t lobe) const noexcept override {
        return true;
    }

    /**
     * @brief Evaluates the BSDF. Accounts for the cosine foreshortening term.
     */
    [[nodiscard]] bsdf_result_t f(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept override {
        return bsdf_result_t{};
    }

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
            const bsdf_query_t& query) const noexcept override {
        return solid_angle_density_t{};
    }

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
