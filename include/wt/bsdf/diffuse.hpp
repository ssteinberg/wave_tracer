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

#include <wt/texture/texture.hpp>
#include <wt/wt_context.hpp>

#include "bsdf.hpp"

namespace wt::bsdf {

/**
 * @brief Idealized Lambertian (perfectly-diffusing) interface. 
 *        Diffuse BSDF admit no transmission: all light is diffusely scattered into the upper hemisphere.
 */
class diffuse_t final : public bsdf_t {
private:
    std::shared_ptr<texture::texture_t> refl;

public:
    diffuse_t(std::string id, std::shared_ptr<texture::texture_t> reflectance)
        : bsdf_t(std::move(id)), refl(std::move(reflectance)) {}
    diffuse_t(diffuse_t &&) = default;

    /**
     * @brief Spectral albedo. Returns std::nullopt when albedo cannot be computed.
     * @param k wavenumber
     */
    [[nodiscard]] inline std::optional<f_t> albedo(const wavenumber_t k) const noexcept override {
        return refl->mean_value(k);
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
    [[nodiscard]] inline bool is_delta_only(const wavenumber_t k) const noexcept override {
        return false;
    }
    
    /**
     * @brief Is a lobe a delta lobe?
     */
    [[nodiscard]] inline bool is_delta_lobe(wavenumber_t k, std::uint32_t lobe) const noexcept override {
        return false;
    }

    /**
    * @brief Returns true for BSDF that make use of the surface interaction footprint data
    */
    [[nodiscard]] inline bool needs_interaction_footprint() const noexcept override {
        return refl->needs_interaction_footprint();
    }

    /**
    * @brief Evaluates the BSDF. Accounts for the cosine foreshortening term.
    */
    [[nodiscard]] bsdf_result_t f(
            const dir3_t &wi, 
            const dir3_t &wo,
            const bsdf_query_t &query) const noexcept override;

    /**
    * @brief Samples the BSDF. The returned weight is bsdf/pdf.
        * @param S normalized Stokes parameters vector that describes the polarimetric properties of incident radiation.
    */
    [[nodiscard]] std::optional<bsdf_sample_t> sample(
            const dir3_t &wi, 
            const bsdf_query_t &query,
            sampler::sampler_t &sampler) const noexcept override;

    /**
    * @brief Provides the sample density.
        * @param S normalized Stokes parameters vector that describes the polarimetric properties of incident radiation.
    */
    [[nodiscard]] solid_angle_density_t pdf(
            const dir3_t &wi, const dir3_t &wo,
            const bsdf_query_t &query) const noexcept override;

    /**
    * @brief Computes the refractive-index ratio: eta at exit / eta at entry.
    * @param k wavenumber
    */
    [[nodiscard]] f_t eta(const dir3_t &wi, const dir3_t &wo,
                          const wavenumber_t k) const noexcept override {
        return 1;
    }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::unique_ptr<bsdf_t> load(std::string id, scene::loader::loader_t* loader,
                                        const scene::loader::node_t& node,
                                        const wt::wt_context_t &context);
};

} // namespace wt::bsdf
