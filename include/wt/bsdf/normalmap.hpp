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

#include <wt/wt_context.hpp>

#include <wt/texture/texture.hpp>
#include "bsdf.hpp"

namespace wt::bsdf {

/**
 * @brief Normal mapping BSDF.
 *        A nested texture encodes the shading normal variations ([.5,.5,1.] encodes an unchanged normal).
 *        Bitmap textures should use linear colour conding.
 */
class normalmap_t final : public bsdf_t {
private:
    std::shared_ptr<texture::texture_t> normalmap;
    std::shared_ptr<bsdf_t> nested;
    bool flip = false;

public:
    normalmap_t(std::string id,
                std::shared_ptr<texture::texture_t> normalmap,
                std::shared_ptr<bsdf_t> nested,
                bool flip)
        : bsdf_t(std::move(id)), 
          normalmap(std::move(normalmap)),
          nested(std::move(nested)),
          flip(flip)
    {}
    normalmap_t(normalmap_t&&) = default;

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
        // query normal map
        const auto rgba = normalmap->get_RGBA(tquery);
        const auto nmn = vec3_t{ rgba.x*2-1, rgba.y*2-1, rgba.z*2-1 } * 
            (flip ? vec3_t{ -1,-1,1 } : vec3_t{ 1,1,1 });
        // build local normal-mapped shading frame
        const dir3_t n = m::normalize(nmn);

        // perturb the world shading normal by the normalmap
        const auto sworld = nested->shading_frame(tquery, tangent_frame, ns);
        return nested->shading_frame(tquery, tangent_frame, sworld.to_world(n));
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
        return nested->needs_interaction_footprint() || normalmap->needs_interaction_footprint();
    }

    /**
     * @brief Evaluates the BSDF. Accounts for the cosine foreshortening term.
     */
    [[nodiscard]] bsdf_result_t f(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept override {
        return nested->f(wi,wo,query);
    }

    /**
     * @brief Samples the BSDF. The returned weight is bsdf/pdf. 
     * @param S normalized Stokes parameters vector that describes the polarimetric properties of incident radiation.
     */
    [[nodiscard]] std::optional<bsdf_sample_t> sample(
            const dir3_t &wi,
            const bsdf_query_t& query, 
            sampler::sampler_t& sampler) const noexcept override {
        return nested->sample(wi,query,sampler);
    }
    
    /**
     * @brief Provides the sample density. 
     * @param S normalized Stokes parameters vector that describes the polarimetric properties of incident radiation.
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
