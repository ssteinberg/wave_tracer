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

#include <wt/spectrum/spectrum.hpp>
#include <wt/spectrum/uniform.hpp>

#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

#include <wt/texture/texture.hpp>

namespace wt::texture {

/**
 * @brief Constant (real-valued) texture.
 */
class constant_t final : public texture_t {
private:
    std::shared_ptr<spectrum::spectrum_real_t> spectrum;

public:
    constant_t(std::string id, std::shared_ptr<spectrum::spectrum_real_t> spectrum)
        : texture_t(std::move(id)),
          spectrum(std::move(spectrum))
    {}
    constant_t(std::string id, f_t value)
        : texture_t(std::move(id)),
          spectrum(std::make_shared<spectrum::uniform_t>(get_id()+"_constant", value))
    {}
    constant_t(constant_t&&) = default;
    virtual ~constant_t() noexcept = default;

    [[nodiscard]] inline const auto& get_spectrum() const noexcept { return spectrum; }
    
    /**
     * @brief Returns texture resolution represented via \f$ \frac{\text{texels}}{\vec{uv}} \f$. May return an approximation.
     *        Can be infinite, e.g., for analytic textures. Can be 1, for constant textures.
     */
    [[nodiscard]] vec2_t resolution() const noexcept override {
        return vec2_t{ 1 };
    }

    /**
     * @brief Average spectrum of the texture. Returns ``nullptr`` when an average spectrum cannot be computed.
     */
    [[nodiscard]] std::shared_ptr<spectrum::spectrum_real_t> mean_spectrum() const noexcept override {
        return spectrum;
    }

    /**
     * @brief Average value of the texture. Returns std::nullopt when an average value cannot be computed.
     */
    [[nodiscard]] std::optional<f_t> mean_value(wavenumber_t k) const noexcept override { return spectrum->f(k); }

    /**
     * @brief Samples the texture.
     *        Returns filtered RGBA data, without spectral upsampling. Ignores ``query.k``.
     *        Only relevant for some textures.
     */
    [[nodiscard]] vec4_t get_RGBA(const texture_query_t& query) const noexcept override {
        // not implemented
        assert(false);
        return {};
    }

    /**
     * @brief Samples the texture. Returns spectrally upsampled (to wavenumber ``query.k``) result.
     * @return Spectral luminance value and alpha (if any) pair.
     */
    [[nodiscard]] vec2_t f(const texture_query_t& query) const noexcept override {
        return { spectrum->f(query.k),1 };
    }
    
public:
    static std::unique_ptr<texture_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
