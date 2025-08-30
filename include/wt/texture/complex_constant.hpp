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
#include <wt/texture/complex.hpp>
#include <wt/spectrum/spectrum.hpp>

#include <wt/interaction/intersection.hpp>
#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

namespace wt::texture {

/**
 * @brief Constant (complex-valued) texture.
 */
class complex_constant_t final : public complex_t {
private:
    std::shared_ptr<spectrum::spectrum_t> spectrum;

public:
    complex_constant_t(std::string id,
                       std::shared_ptr<spectrum::spectrum_t>&& spectrum)
        : complex_t(std::move(id)),
          spectrum(std::move(spectrum))
    {}
    complex_constant_t(complex_constant_t&&) = default;
    virtual ~complex_constant_t() noexcept = default;
    
    /**
     * @brief Returns TRUE for textures that make use of the surface interaction footprint data.
     */
    [[nodiscard]] bool needs_interaction_footprint() const noexcept override {
        return false;
    }
    
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
    [[nodiscard]] std::shared_ptr<spectrum::spectrum_t> mean_spectrum() const noexcept override {
        return spectrum;
    }

    /**
     * @brief Average value of the texture. Returns std::nullopt when an average value cannot be computed.
     */
    [[nodiscard]] std::optional<c_t> mean_value(wavenumber_t k) const noexcept override {
        return spectrum->value(k);
    }

    /**
     * @brief Samples the texture.
     */
    [[nodiscard]] c_t f(const texture_query_t& query) const noexcept override {
        return spectrum->value(query.k);
    }
    
public:
    static std::shared_ptr<complex_t> load(
            std::string id, 
            scene::loader::loader_t* loader, 
            const scene::loader::node_t& node, 
            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
