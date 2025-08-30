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
#include <wt/texture/texture.hpp>
#include <wt/texture/complex.hpp>

#include <wt/spectrum/complex_container.hpp>

#include <wt/interaction/intersection.hpp>
#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

namespace wt::texture {

/**
 * @brief Complex-valued texture. Uses a pair of (real-valued) textures: one for the real part and one for the imaginary part.
 */
class complex_container_t final : public complex_t {
private:
    std::shared_ptr<texture_t> real_texture, imag_texture;
    std::shared_ptr<spectrum::complex_container_t> avg_spectrum;

public:
    complex_container_t(std::string id,
              std::shared_ptr<texture_t> real_texture, 
              std::shared_ptr<texture_t> imag_texture) 
        : complex_t(std::move(id)),
          real_texture(std::move(real_texture)),
          imag_texture(std::move(imag_texture))
    {
        const auto& rms = this->real_texture->mean_spectrum();
        const auto& ims = this->imag_texture->mean_spectrum();
        if (rms && ims) {
            this->avg_spectrum = std::make_shared<spectrum::complex_container_t>(
                    this->get_id() + "_mean_spectrum", rms, ims);
        }
    }
    complex_container_t(complex_container_t&&) = default;
    virtual ~complex_container_t() noexcept = default;
    
    /**
     * @brief Returns TRUE for textures that make use of the surface interaction footprint data.
     */
    [[nodiscard]] bool needs_interaction_footprint() const noexcept override {
        return real_texture->needs_interaction_footprint() || imag_texture->needs_interaction_footprint();
    }
    
    /**
     * @brief Returns texture resolution represented via \f$ \frac{\text{texels}}{\vec{uv}} \f$. May return an approximation.
     *        Can be infinite, e.g., for analytic textures. Can be 1, for constant textures.
     */
    [[nodiscard]] vec2_t resolution() const noexcept override {
        return m::max(real_texture->resolution(), imag_texture->resolution());
    }

    /**
     * @brief Average spectrum of the texture. Returns ``nullptr`` when an average spectrum cannot be computed.
     */
    [[nodiscard]] std::shared_ptr<spectrum::spectrum_t> mean_spectrum() const noexcept override {
        return avg_spectrum;
    }

    /**
     * @brief Average value of the texture. Returns std::nullopt when an average value cannot be computed.
     */
    [[nodiscard]] std::optional<c_t> mean_value(wavenumber_t k) const noexcept override {
        const auto rmv = real_texture->mean_value(k);
        const auto imv = imag_texture->mean_value(k);
        if (!rmv || !imv)
            return std::nullopt;
        return c_t{ *rmv, *imv };
    }

    /**
     * @brief Samples the texture.
     */
    [[nodiscard]] c_t f(const texture_query_t& query) const noexcept override {
        return c_t{
            real_texture->f(query).x,
            imag_texture->f(query).x
        };
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
