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
 * @brief Scales the nested BSDF by a supplied texture. 
 */
class scale_t final : public bsdf_t {
private:
    std::shared_ptr<texture::texture_t> scale;
    std::shared_ptr<bsdf_t> nested;

public:
    scale_t(std::string id, std::shared_ptr<texture::texture_t> scale, std::shared_ptr<bsdf_t> nested)
        : bsdf_t(std::move(id)), 
          scale(std::move(scale)),
          nested(std::move(nested)) 
    {}
    scale_t(scale_t&&) = default;

    /**
     * @brief Constructs a shading frame in world space.
     *        This is useful for BSDFs that perturb the shading frame, like normal or bump maps.
     *
     * @param tquery texture query data.
     * @param tangent_frame mesh tangent frame at intersection.
     * @param ns interpolated shading normal at intersection.
     */
    [[nodiscard]] frame_t shading_frame(
            const texture::texture_query_t& tquery,
            const mesh::surface_differentials_t& tangent_frame,
            const dir3_t& ns) const noexcept override {
        return nested->shading_frame(tquery, tangent_frame, ns);
    }

    /**
     * @brief Spectral albedo. Returns std::nullopt when albedo cannot be computed.
     *        Returns an approximation.
     * @param k wavenumber
     */
    [[nodiscard]] inline std::optional<f_t> albedo(const wavenumber_t k) const noexcept override {
        const auto nested_mv = nested->albedo(k);
        const auto scale_mv = scale->mean_value(k);
        if (!nested_mv || !scale_mv)
            return std::nullopt;
        return *nested_mv * *scale_mv;
    }
    
    /**
     * @brief Returns mask of all available lobes for this BSDF at particular wavenumber.
     */
    [[nodiscard]] lobe_mask_t lobes(wavenumber_t k) const noexcept override {
        return nested->lobes(k);
    }
    
    /**
     * @brief Does this BSDF comprise of only delta lobes?
     */
    [[nodiscard]] inline bool is_delta_only(wavenumber_t k) const noexcept override {
        return nested->is_delta_only(k);
    }
    
    /**
     * @brief Is a lobe a delta lobe?
     */
    [[nodiscard]] inline bool is_delta_lobe(wavenumber_t k, std::uint32_t lobe) const noexcept override {
        return nested->is_delta_lobe(k,lobe);
    }
    
    /**
     * @brief Returns true for BSDF that make use of the surface interaction footprint data
     */
    [[nodiscard]] inline bool needs_interaction_footprint() const noexcept override {
        return nested->needs_interaction_footprint() ||
               scale->needs_interaction_footprint();
    }

    /**
     * @brief Evaluates the BSDF. Accounts for the cosine foreshortening term.
     */
    [[nodiscard]] bsdf_result_t f(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept override {
        const auto& tquery = query.intersection.texture_query(query.k);
        auto ret = nested->f(wi,wo,query);
        ret.M *= scale->f(tquery).x;
        return ret;
    }

    /**
     * @brief Samples the BSDF. The returned weight is bsdf/pdf. 
     */
    [[nodiscard]] std::optional<bsdf_sample_t> sample(
            const dir3_t &wi,
            const bsdf_query_t& query, 
            sampler::sampler_t& sampler) const noexcept override {
        const auto& tquery = query.intersection.texture_query(query.k);
        auto s = nested->sample(wi,query,sampler);
        if (s)
            s->weighted_bsdf.M *= scale->f(tquery).x;
        return s;
    }
    
    /**
     * @brief Provides the sample density. 
     */
    [[nodiscard]] solid_angle_density_t pdf(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept override {
        return nested->pdf(wi,wo,query);
    }

    /**
     * @brief Computes the refractive-index ratio: eta at exit / eta at entry.
     * @param k wavenumber
     */
    [[nodiscard]] f_t eta(
            const dir3_t &wi,
            const dir3_t &wo,
            const wavenumber_t k) const noexcept override {
        return nested->eta(wi,wo,k);
    }

    [[nodiscard]] inline const auto& nested_bsdf() const noexcept { return nested; }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::unique_ptr<bsdf_t> load(std::string id, 
                                        scene::loader::loader_t* loader, 
                                        const scene::loader::node_t& node, 
                                        const wt::wt_context_t &context);
};

}
