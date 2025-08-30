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
 * @brief (Real-valued) texture that linearly interpolates between a pair of textures using a third texture as weight.
 */
class mix_t final : public texture_t {
private:
    std::shared_ptr<texture_t> texture1, texture2, mix;

public:
    mix_t(std::string id, 
            std::shared_ptr<texture_t> tex1, 
            std::shared_ptr<texture_t> tex2,
            std::shared_ptr<texture_t> mix)
        : texture_t(std::move(id)),
          texture1(std::move(tex1)),
          texture2(std::move(tex2)),
          mix(std::move(mix))
    {}
    mix_t(mix_t&&) = default;
    virtual ~mix_t() noexcept = default;
    
    /**
     * @brief Returns TRUE for textures that make use of the surface interaction footprint data.
     */
    [[nodiscard]] bool needs_interaction_footprint() const noexcept override {
        return texture1->needs_interaction_footprint() || texture2->needs_interaction_footprint() ||
               mix->needs_interaction_footprint();
    }
    
    /**
     * @brief Returns texture resolution represented via \f$ \frac{\text{texels}}{\vec{uv}} \f$. May return an approximation.
     *        Can be infinite, e.g., for analytic textures. Can be 1, for constant textures.
     */
    [[nodiscard]] vec2_t resolution() const noexcept override {
        return m::max(texture1->resolution(), texture2->resolution(), mix->resolution());
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
        const auto mv1 = texture1->mean_value(k);
        const auto mv2 = texture2->mean_value(k);
        const auto mix = this->mix->mean_value(k);
        if (!mv1 || !mv2 || !mix)
            return std::nullopt;
        return m::mix(*mv1,*mv2, *mix);
    }

    /**
     * @brief Samples the texture.
     *        Returns filtered RGBA data, without spectral upsampling. Ignores ``query.k``.
     *        Only relevant for some textures.
     */
    [[nodiscard]] vec4_t get_RGBA(const texture_query_t& query) const noexcept override {
        const auto m = mix->get_RGBA(query).x;

        const auto t1 = m!=1 ? texture1->get_RGBA(query) : vec4_t{};
        if (m==0) return t1;
        
        const auto t2=texture2->get_RGBA(query);
        if (m==1) return t2;

        return m::mix(t1,t2, m);
    }

    /**
     * @brief Samples the texture. Returns spectrally upsampled (to wavenumber ``query.k``) result.
     * @return Spectral luminance value and alpha (if any) pair.
     */
    [[nodiscard]] vec2_t f(const texture_query_t& query) const noexcept override {
        const auto m = mix->f(query).x;

        const auto t1 = m!=1 ? texture1->f(query) : vec2_t{};
        if (m==0) return t1;
        
        const auto t2=texture2->f(query);
        if (m==1) return t2;

        return m::mix(t1,t2, m);
    }
    
public:
    static std::unique_ptr<texture_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
