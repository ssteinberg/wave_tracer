/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <future>
#include <string>
#include <memory>

#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

#include <wt/spectrum/colourspace/RGB/RGB_to_spectral.hpp>
#include <wt/texture/texture.hpp>

#include <wt/bitmap/texture2d.hpp>

namespace wt::texture {

/**
 * @brief Texture defined via a 2D bitmap.
 *        Texture filtering is configurable, see `wt::bitmap::texture2d_config_t`.
 */
class bitmap_t final : public texture_t {
public:
    using texture2d_t = bitmap::texture2d_t;

private:
    struct private_token_t {};
    std::future<void> defered_load_future;

    /**
     * @brief Name of the auxiliary loading task for this bitmap.
     */
    [[nodiscard]] inline std::string aux_task_name() noexcept {
        return "bitmap_t:" + get_id();
    }

private:
    std::unique_ptr<texture2d_t> tex;
    std::shared_ptr<spectrum::spectrum_real_t> avg_spectrum;

public:
    bitmap_t(std::string id, private_token_t)
        : texture_t(std::move(id))
    {}
    bitmap_t(bitmap_t&&) = default;
    virtual ~bitmap_t() noexcept = default;

    /**
     * @brief Returns the underlying 2D texture.
     */
    [[nodiscard]] const auto& get_texture() const noexcept {
        return tex;
    }
    
    /**
     * @brief Returns texture resolution represented via \f$ \frac{\text{texels}}{\vec{uv}} \f$. May return an approximation.
     *        Can be infinite, e.g., for analytic textures. Can be 1, for constant textures.
     */
    [[nodiscard]] vec2_t resolution() const noexcept override {
        return vec2_t{ get_texture()->dimensions() };
    }

    /**
     * @brief Average spectrum of the texture. Returns ``nullptr`` when an average spectrum cannot be computed.
     */
    [[nodiscard]] std::shared_ptr<spectrum::spectrum_real_t> mean_spectrum() const noexcept override {
        return avg_spectrum;
    }

    /**
     * @brief Returns the underlying texture filering configuration.
     */
    [[nodiscard]] auto get_filter_config() const noexcept { return get_texture()->get_tex_filter(); }

    /**
     * @brief Returns TRUE if the underlying bitmap contains RGB or RGBA data.
     */
    [[nodiscard]] bool is_RGB() const noexcept {
        const auto& tex = get_texture();
        return tex->pixel_layout() == bitmap::pixel_layout_e::RGB ||
               tex->pixel_layout() == bitmap::pixel_layout_e::RGBA;
    }

    /**
     * @brief Average value of the texture. Returns std::nullopt when an average value cannot be computed.
     */
    [[nodiscard]] std::optional<f_t> mean_value(wavenumber_t k) const noexcept override {
        const auto& tex = get_texture();
        const auto& avg4 = tex->mean_value();
        if (is_RGB()) {
            // upscale RGB to spectral
            const auto lambda = wavenum_to_wavelen(k);
            return colourspace::RGB_to_spectral::uplift(vec3_t{ avg4 }, lambda);
        } else {
            return avg4.x;
        }
    }

    /**
     * @brief Samples the texture.
     *        Returns filtered RGBA data, without spectral upsampling. Ignores ``query.k``.
     *        Only relevant for some textures.
     */
    [[nodiscard]] vec4_t get_RGBA(const texture_query_t& query) const noexcept override {
        const auto& tex = get_texture();
        // filter the texture
        const auto val4 = tex->filter(query.uv, 
                                      vec2_t{ query.pdvs.duda, query.pdvs.dudb }, 
                                      vec2_t{ query.pdvs.dvda, query.pdvs.dvdb });
        return val4;
    }

    /**
     * @brief Samples the texture. Returns spectrally upsampled (to wavenumber ``query.k``) result.
     * @return Spectral luminance value and alpha (if any) pair.
     */
    [[nodiscard]] vec2_t f(const texture_query_t& query) const noexcept override {
        const auto val4 = get_RGBA(query);

        if (is_RGB()) {
            // upscale RGB to spectral
            const auto lambda = wavenum_to_wavelen(query.k);
            const auto L = colourspace::RGB_to_spectral::uplift(vec3_t{ val4 }, lambda);
            assert(m::isfinite(L));

            return { L, val4.w };
        } else {
            // return constant value for luminance textures
            return { val4.x, val4.w };
        }
    }

public:
    static std::shared_ptr<bitmap_t> load(std::string id, 
                                          scene::loader::loader_t* loader, 
                                          const scene::loader::node_t& node, 
                                          const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
