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

#include <wt/wt_context.hpp>
#include <wt/math/common.hpp>
#include <wt/math/transform/transform.hpp>

#include <wt/texture/texture.hpp>

#include <wt/util/assert.hpp>

namespace wt::texture {

/**
 * @brief Affine transforms the UV texture coordinates before sampling an underlying texture.
 */
class transform_t final : public texture_t {
private:
    std::shared_ptr<texture_t> tex;
    mat2_t M;
    vec2_t translate;

private:
    [[nodiscard]] auto transform_query(const texture_query_t& query) const noexcept {
        // transform uv
        const auto uv = M*query.uv + translate;

        // new query
        auto q = texture_query_t{
            .uv = uv,
            .k = query.k,
        };

        // transform uv pdvs
        if (needs_interaction_footprint()) {
            const auto duvda = M * vec2_t{ query.pdvs.duda, query.pdvs.dvda };
            const auto duvdb = M * vec2_t{ query.pdvs.dudb, query.pdvs.dvdb };

            q.pdvs.duda = duvda.x;
            q.pdvs.dudb = duvdb.x;
            q.pdvs.dvda = duvda.y;
            q.pdvs.dvdb = duvdb.y;
        }

        return q;
    }

public:
    transform_t(std::string id, 
                std::shared_ptr<texture_t> tex, 
                const mat2_t& M,
                const vec2_t translate)
        : texture_t(std::move(id)),
          tex(std::move(tex)),
          M(M),
          translate(translate)
    {
        assert_isnotzero(m::determinant(M));
    }
    transform_t(transform_t&&) = default;
    virtual ~transform_t() noexcept = default;

    [[nodiscard]] inline const auto* get_nested_texture() const noexcept {
        return tex.get();
    }
    
    /**
     * @brief Returns TRUE for textures that make use of the surface interaction footprint data.
     */
    [[nodiscard]] bool needs_interaction_footprint() const noexcept override {
        return tex->needs_interaction_footprint();
    }
    
    /**
     * @brief Returns texture resolution represented via \f$ \frac{\text{texels}}{\vec{uv}} \f$. May return an approximation.
     *        Can be infinite, e.g., for analytic textures. Can be 1, for constant textures.
     */
    [[nodiscard]] vec2_t resolution() const noexcept override {
        const auto r = m::max(vec2_t{ 1 }, tex->resolution());
        const auto rr = M * (f_t(1)/r);

        return m::max(vec2_t{ 1 }, f_t(1)/rr);
    }

    /**
     * @brief Average spectrum of the texture. Returns ``nullptr`` when an average spectrum cannot be computed.
     */
    [[nodiscard]] std::shared_ptr<spectrum::spectrum_real_t> mean_spectrum() const noexcept override {
        return tex->mean_spectrum();
    }

    /**
     * @brief Average value of the texture. Returns std::nullopt when an average value cannot be computed.
     */
    [[nodiscard]] std::optional<f_t> mean_value(wavenumber_t k) const noexcept override {
        return tex->mean_value(k);
    }

    /**
     * @brief Samples the texture.
     *        Returns filtered RGBA data, without spectral upsampling. Ignores ``query.k``.
     *        Only relevant for some textures.
     */
    [[nodiscard]] vec4_t get_RGBA(const texture_query_t& query) const noexcept override {
        return tex->get_RGBA(transform_query(query));
    }

    /**
     * @brief Samples the texture. Returns spectrally upsampled (to wavenumber ``query.k``) result.
     * @return Spectral luminance value and alpha (if any) pair.
     */
    [[nodiscard]] vec2_t f(const texture_query_t& query) const noexcept override {
        return tex->f(transform_query(query));
    }
    
public:
    static std::unique_ptr<texture_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
