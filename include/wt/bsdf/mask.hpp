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
 * @brief Masks the nested BSDF using the opacity of the supplied texture. 
 *        Masked samples are null delta lobes (perfect forward "transmission").
 *        Transmissive nested BSDFs are NOT supported.
 */
class mask_t final : public bsdf_t {
private:
    std::shared_ptr<texture::texture_t> mask;
    std::shared_ptr<bsdf_t> nested;

    [[nodiscard]] inline auto lobe_null(const wavenumber_t k) const {
        const auto ls = nested->lobes(k);
        for (auto l=(int)ls.size()-1;;--l) {
            if (!ls.test(l))
                return (std::uint32_t)l;
            if (l==0)
                throw std::runtime_error("(mask bsdf) nested BSDF admits no empty lobes.");
        }
    }

public:
    mask_t(std::string id, std::shared_ptr<texture::texture_t> mask, std::shared_ptr<bsdf_t> nested)
        : bsdf_t(std::move(id)), mask(std::move(mask)), nested(std::move(nested))
    {}
    mask_t(mask_t &&) = default;

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
     * @param k wavenumber
     */
    [[nodiscard]] inline std::optional<f_t> albedo(const wavenumber_t k) const noexcept override {
        return nested->albedo(k);
    }
    
    /**
     * @brief Returns mask of all available lobes for this BSDF at particular wavenumber.
     */
    [[nodiscard]] lobe_mask_t lobes(wavenumber_t k) const noexcept override {
        auto lobes = nested->lobes(k);
        lobes.set(lobe_null(k));
        return lobes;
    }

    /**
     * @brief Does this BSDF comprise of only delta lobes?
     */
    [[nodiscard]] inline bool is_delta_only(const wavenumber_t k) const noexcept override {
        return nested->is_delta_only(k);
    }
    
    /**
     * @brief Is a lobe a delta lobe?
     */
    [[nodiscard]] inline bool is_delta_lobe(wavenumber_t k, std::uint32_t lobe) const noexcept override {
        if (lobe==lobe_null(k))
            return true;
        return nested->is_delta_lobe(k,lobe);
    }
    
    /**
     * @brief Returns true for BSDF that make use of the surface interaction footprint data
     */
    [[nodiscard]] inline bool needs_interaction_footprint() const noexcept override {
        return nested->needs_interaction_footprint() || mask->needs_interaction_footprint();
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
        // no transmission on masked nested bsdf
        return 1;
    }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::unique_ptr<bsdf_t> load(std::string id, scene::loader::loader_t* loader,
                                        const scene::loader::node_t& node,
                                        const wt::wt_context_t &context);
};

} // namespace wt::bsdf
