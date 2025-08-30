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

#include <wt/scene/element/scene_element.hpp>
#include <wt/sampler/sampler.hpp>
#include <wt/texture/texture.hpp>
#include <wt/wt_context.hpp>

#include <wt/math/common.hpp>

namespace wt::surface_profile {

using texture::texture_query_t;

struct surface_profile_sample_ret_t {
    dir3_t wo;
    f_t pdf, psd;
    f_t weight;   // psd/pdf
};

/**
 * @brief Quantifies the statistical profile of a surface.
 *        The primary quantity is the surface's power spectral density (PSD), characterizing effective roughness.
 *        Surface profiles are wavelength dependent.
 */
class surface_profile_t : public scene::scene_element_t {
public:
    static constexpr std::string scene_element_class() noexcept { return "surface_profile"; }

    using rms_t = quantity<inverse(isq::length[u::mm])>;
    using rms2_t = quantity<inverse(isq::area[square(u::mm)])>;
    using recp_rms_t = quantity<isq::length[u::mm]>;
    using recp_rms2_t = quantity<isq::area[square(u::mm)]>;
    using variance_t = quantity<inverse(isq::area[square(u::mm)])>;

public:
    surface_profile_t(std::string id) : scene_element_t(std::move(id)) {}
    surface_profile_t(surface_profile_t&&) = default;
    virtual ~surface_profile_t() noexcept = default;

    [[nodiscard]] virtual bool is_delta_only(wavenumber_t k) const noexcept = 0;
    
    /**
     * @brief Returns true for profiles that make use of the surface interaction footprint data
     */
    [[nodiscard]] virtual bool needs_interaction_footprint() const noexcept = 0;

    /**
     * @brief Variance of the surface profile.
     */
    [[nodiscard]] virtual variance_t variance(const texture_query_t& query) const noexcept = 0;

    /**
     * @brief RMS roughness of the surface profile.
     */
    [[nodiscard]] virtual rms_t rms_roughness(const texture_query_t& query) const noexcept = 0;

    /**
     * @brief Fraction of scatter contained in specular term.
     * @param k wavenumber
     */
    [[nodiscard]] virtual f_t alpha(const dir3_t& wi,
                                   const dir3_t& wo,
                                   const texture_query_t& query) const noexcept = 0;
    /**
     * @brief Fraction of scatter contained in specular term.
     * @param k wavenumber
     */
    [[nodiscard]] virtual f_t alpha(const dir3_t& wi,
                                    const texture_query_t& query) const noexcept = 0;

    /**
     * @brief Evaluates the surface profile power spectral density (PSD).
     */
    [[nodiscard]] virtual f_t psd(const dir3_t& wi,
                                  const dir3_t& wo,
                                  const texture_query_t& query) const noexcept = 0;

    /**
     * @brief Samples the surface profile.
     */
    [[nodiscard]] virtual surface_profile_sample_ret_t sample(
            const dir3_t &wi,
            const texture_query_t& query,
            sampler::sampler_t& sampler) const noexcept = 0;
    
    /**
     * @brief Provides the sampling density. 
     */
    [[nodiscard]] virtual f_t pdf(const dir3_t& wi,
                                  const dir3_t& wo,
                                  const texture_query_t& query) const noexcept = 0;

public:
    static std::unique_ptr<surface_profile_t> load(std::string id, scene::loader::loader_t* loader, const scene::loader::node_t& node, const wt::wt_context_t &context);

    [[nodiscard]] virtual scene::element::info_t description() const override = 0;
};

}
