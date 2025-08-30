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

#include <wt/interaction/surface_profile/surface_profile.hpp>
#include <wt/math/common.hpp>

namespace wt::surface_profile {

/**
 * @brief A perfectly smooth surface with Dirac delta statistics. 
 */
class dirac_t final : public surface_profile_t {
public:
    dirac_t(std::string id) noexcept 
        : surface_profile_t(std::move(id))
    {}

    /**
     * @brief Variance of the surface profile.
     */
    [[nodiscard]] variance_t variance(const texture_query_t& query) const noexcept override {
        return variance_t::zero();
    }

    /**
     * @brief RMS roughness of the surface profile.
     */
    [[nodiscard]] rms_t rms_roughness(const texture_query_t& query) const noexcept override {
        return rms_t::zero();
    }

    /**
     * @brief Fraction of scatter contained in specular term.
     * @param k wavenumber
     */
    [[nodiscard]] f_t alpha(const dir3_t& wi,
                            const dir3_t& wo,
                            const texture_query_t& query) const noexcept override {
        return 1;
    }
    /**
     * @brief Fraction of scatter contained in specular term.
     * @param k wavenumber
     */
    [[nodiscard]] f_t alpha(const dir3_t& wi,
                            const texture_query_t& query) const noexcept override {
        return 1;
    }

    [[nodiscard]] bool is_delta_only(wavenumber_t k) const noexcept override {
        return true;
    }
    
    /**
     * @brief Returns true for profiles that make use of the surface interaction footprint data
     */
    [[nodiscard]] bool needs_interaction_footprint() const noexcept override {
        return false;
    }

    /**
     * @brief Evaluates the dirac lobe power spectral density (PSD).
     */
    [[nodiscard]] f_t psd(const dir3_t& wi,
                          const dir3_t& wo,
                          const texture_query_t& query) const noexcept override {
        return 0;
    }

    /**
     * @brief Samples a scattered direction.
     */
    [[nodiscard]] surface_profile_sample_ret_t sample(
            const dir3_t& wi,
            const texture_query_t& query,
            sampler::sampler_t& sampler) const noexcept override {
        // should not be called: delta only
        return surface_profile_sample_ret_t{ .wo = dir3_t{ 0,0,1 } };
    }

    /**
     * @brief Provides the sampling density. 
     */
    [[nodiscard]] f_t pdf(const dir3_t& wi,
                          const dir3_t& wo,
                          const texture_query_t& query) const noexcept override {
        // should not be called: delta only
        return 0;
    }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::unique_ptr<surface_profile_t> load(std::string id, scene::loader::loader_t* loader, const scene::loader::node_t& node, const wt::wt_context_t &context);
};

}
