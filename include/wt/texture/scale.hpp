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
 * @brief Scales a nested texture by another texture.
 */
class scale_t final : public texture_t {
private:
    std::shared_ptr<texture_t> tex, scale;

public:
    scale_t(std::string id, 
            std::shared_ptr<texture_t> tex, 
            std::shared_ptr<texture_t> scale)
        : texture_t(std::move(id)),
          tex(std::move(tex)),
          scale(std::move(scale))
    {}
    scale_t(scale_t&&) = default;
    virtual ~scale_t() noexcept = default;

    [[nodiscard]] inline const auto* get_nested_texture() const noexcept {
        return tex.get();
    }
    [[nodiscard]] inline const auto* get_scale_texture() const noexcept {
        return scale.get();
    }
    
    /**
     * @brief Returns TRUE for textures that make use of the surface interaction footprint data.
     */
    [[nodiscard]] bool needs_interaction_footprint() const noexcept override {
        return tex->needs_interaction_footprint() || scale->needs_interaction_footprint();
    }
    
    /**
     * @brief Returns texture resolution represented via \f$ \frac{\text{texels}}{\vec{uv}} \f$. May return an approximation.
     *        Can be infinite, e.g., for analytic textures. Can be 1, for constant textures.
     */
    [[nodiscard]] vec2_t resolution() const noexcept override {
        return m::max(tex->resolution(), scale->resolution());
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
     *        (returns an approximation)
     */
    [[nodiscard]] std::optional<f_t> mean_value(wavenumber_t k) const noexcept override {
        const auto tmv = tex->mean_value(k);
        const auto smv = scale->mean_value(k);
        if (!tmv || !smv)
            return std::nullopt;
        return *tmv * *smv;
    }

    /**
     * @brief Samples the texture.
     *        Returns filtered RGBA data, without spectral upsampling. Ignores ``query.k``.
     *        Only relevant for some textures.
     */
    [[nodiscard]] vec4_t get_RGBA(const texture_query_t& query) const noexcept override {
        return tex->get_RGBA(query) * scale->get_RGBA(query);
    }

    /**
     * @brief Samples the texture. Returns spectrally upsampled (to wavenumber ``query.k``) result.
     * @return Spectral luminance value and alpha (if any) pair.
     */
    [[nodiscard]] vec2_t f(const texture_query_t& query) const noexcept override {
        return tex->f(query) * scale->f(query);
    }
    
public:
    static std::unique_ptr<texture_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
