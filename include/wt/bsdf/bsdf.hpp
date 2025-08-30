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
#include <optional>

#include <wt/math/common.hpp>
#include <wt/sampler/measure.hpp>

#include <wt/scene/element/scene_element.hpp>
#include <wt/sampler/sampler.hpp>

#include <wt/wt_context.hpp>

#include "common.hpp"

namespace wt::bsdf {

/**
 * @brief Generic BSDF interface.
 *        BSDFs (bi-directional scattering distribution functions) quantify the interaction of light with an interface.
 */
class bsdf_t : public scene::scene_element_t {
public:
    static constexpr std::string scene_element_class() noexcept { return "bsdf"; }

public:
    bsdf_t(std::string id) : scene_element_t(std::move(id)) {}
    bsdf_t(bsdf_t&&) = default;
    virtual ~bsdf_t() noexcept = default;

    /**
     * @brief Constructs a shading frame in world space.
     *        This is useful for BSDFs that perturb the shading frame, like normal or bump maps.
     *
     * @param tquery texture query data.
     * @param tangent_frame mesh tangent frame at intersection.
     * @param ns interpolated shading normal at intersection.
     */
    [[nodiscard]] virtual frame_t shading_frame(
            const texture::texture_query_t& tquery,
            const mesh::surface_differentials_t& tangent_frame,
            const dir3_t& ns) const noexcept {
        return frame_t::build_shading_frame(ns, tangent_frame.dpdu);
    }

    /**
     * @brief Spectral albedo. Returns std::nullopt when albedo cannot be computed.
     * @param k wavenumber
     */
    [[nodiscard]] virtual std::optional<f_t> albedo(wavenumber_t k) const noexcept = 0;
    
    /**
     * @brief Returns mask of all available lobes for this BSDF at particular wavenumber.
     */
    [[nodiscard]] virtual lobe_mask_t lobes(wavenumber_t k) const noexcept = 0;
    
    /**
     * @brief Returns count of lobes for this BSDF at particular wavenumber.
     */
    [[nodiscard]] int lobe_count(wavenumber_t k) const noexcept {
        return lobes(k).count();
    }
    
    /**
     * @brief Does this BSDF comprise of only delta lobes?
     */
    [[nodiscard]] virtual bool is_delta_only(wavenumber_t k) const noexcept = 0;
    
    /**
     * @brief Is a lobe a delta lobe?
     */
    [[nodiscard]] virtual bool is_delta_lobe(wavenumber_t k, std::uint32_t lobe) const noexcept = 0;
    
    /**
     * @brief Returns true for BSDF that make use of the surface interaction footprint data
     */
    [[nodiscard]] virtual bool needs_interaction_footprint() const noexcept { return false; }

    /**
     * @brief Evaluates the BSDF. Accounts for the cosine foreshortening term. Only non-delta lobes are evaluated.
     * @param wi incident direction (in local frame)
     * @param wo exitant direction (in local frame)
     * @return the polarimetric interaction, quantified by a Mueller matrix; see `wt::bsdf::bsdf_result_t`. Note: the return has implied units of 1/sr.
     */
    [[nodiscard]] virtual bsdf_result_t f(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept = 0;

    /**
     * @brief Samples the BSDF. 
     * @param wi incident direction (in local frame)
     */
    [[nodiscard]] virtual std::optional<bsdf_sample_t> sample(
            const dir3_t &wi,
            const bsdf_query_t& query,
            sampler::sampler_t& sampler) const noexcept = 0;
    
    /**
     * @brief Provides the sample solid angle density of non-delta lobes.
     * @param wi incident direction (in local frame)
     * @param wo exitant direction (in local frame)
     */
    [[nodiscard]] virtual solid_angle_density_t pdf(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept = 0;
    
    /**
     * @brief Computes the refractive-index ratio: eta at exit / eta at entry.
     * @param wi incident direction (in local frame)
     * @param wo exitant direction (in local frame)
     * @param k wavenumber
     */
    [[nodiscard]] virtual f_t eta(
            const dir3_t &wi,
            const dir3_t &wo,
            const wavenumber_t k) const noexcept = 0;

public:
    static std::unique_ptr<bsdf_t> load(std::string id, scene::loader::loader_t* loader, const scene::loader::node_t& node, const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override = 0;
};

}
