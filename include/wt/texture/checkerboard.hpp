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

#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

#include <wt/texture/texture.hpp>

namespace wt::texture {

/**
 * @brief Simple checkerboard pattern.
 */
class checkerboard_t final : public texture_t {
private:
    std::shared_ptr<texture_t> col1, col2;

public:
    checkerboard_t(std::string id, 
                   std::shared_ptr<texture_t> col1,
                   std::shared_ptr<texture_t> col2)
        : texture_t(std::move(id)),
          col1(std::move(col1)),
          col2(std::move(col2))
    {}
    checkerboard_t(checkerboard_t&&) = default;
    virtual ~checkerboard_t() noexcept = default;
    
    /**
     * @brief Returns TRUE for textures that make use of the surface interaction footprint data.
     */
    [[nodiscard]] bool needs_interaction_footprint() const noexcept override {
        return col1->needs_interaction_footprint() || col2->needs_interaction_footprint();
    }
    
    /**
     * @brief Returns texture resolution represented via \f$ \frac{\text{texels}}{\vec{uv}} \f$. May return an approximation.
     *        Can be infinite, e.g., for analytic textures. Can be 1, for constant textures.
     */
    [[nodiscard]] vec2_t resolution() const noexcept override {
        return vec2_t{ m::inf };
    }

    /**
     * @brief Average spectrum of the texture. Returns ``nullptr`` when an average spectrum cannot be computed.
     */
    [[nodiscard]] std::shared_ptr<spectrum::spectrum_real_t> mean_spectrum() const noexcept override {
        // not implemented
        return nullptr;
    }

    /**
     * @brief Average value of the texture. Returns std::nullopt when an average value cannot be computed.
     */
    [[nodiscard]] std::optional<f_t> mean_value(wavenumber_t k) const noexcept override {
        const auto mv1 = col1->mean_value(k);
        const auto mv2 = col2->mean_value(k);
        if (!mv1 || !mv2)
            return std::nullopt;
        return (*mv1 + *mv2) / f_t(2);
    }

    /**
     * @brief Samples the texture.
     *        Returns filtered RGBA data, without spectral upsampling. Ignores ``query.k``.
     *        Only relevant for some textures.
     */
    [[nodiscard]] vec4_t get_RGBA(const texture_query_t& query) const noexcept override {
        const auto uv = query.uv;
        int x = 2*m::modulo(std::int32_t(uv.x), 2) - 1;
        int y = 2*m::modulo(std::int32_t(uv.y), 2) - 1;

        return x*y==1 ?
            col1->get_RGBA(query) :
            col2->get_RGBA(query);
    }

    /**
     * @brief Samples the texture. Returns spectrally upsampled (to wavenumber ``query.k``) result.
     * @return Spectral luminance value and alpha (if any) pair.
     */
    [[nodiscard]] vec2_t f(const texture_query_t& query) const noexcept override {
        const auto uv = query.uv;
        int x = 2*m::modulo(std::int32_t(uv.x), 2) - 1;
        int y = 2*m::modulo(std::int32_t(uv.y), 2) - 1;

        return x*y==1 ?
            col1->f(query) :
            col2->f(query);
    }
    
public:
    static std::unique_ptr<texture_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
