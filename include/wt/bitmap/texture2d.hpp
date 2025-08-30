/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cassert>
#include <utility>

#include <wt/util/unreachable.hpp>

#include <wt/math/common.hpp>
#include <wt/math/norm_integers.hpp>
#include <wt/bitmap/bitmap.hpp>

#include <wt/bitmap/texture2d_storage.hpp>
#include <wt/bitmap/texel_convert.hpp>

#include <wt/bitmap/load2d.hpp>
#include <wt/wt_context.hpp>
#include <wt/scene/element/attributes.hpp>

#include "bitmap_stats.hpp"

namespace wt::bitmap {

/**
 * @brief A texture is a 2D bitmap with filtering and colour encoding facilities.
 */
class alignas(64) texture2d_t {
protected:
    texture2d_storage_t storage;
    texture2d_config_t config;
    /** @brief Colour encoding for unorm/snorm textures.
     *         Ignored for floating point textures.
     */
    colour_encoding_t config_colour_encoding;

    vec4_t min_value, max_value, avg_value;

    inline void compute_texture_data() noexcept {
        min_value = vec4_t{ limits<f_t>::max() };
        max_value = vec4_t{ limits<f_t>::min() };
        avg_value = {};

        const auto dim = storage.info.dim;
        for (std::uint32_t y=0; y<dim.y; ++y) 
        for (std::uint32_t x=0; x<dim.x; ++x) {
            const auto t = texel({ x,y });

            min_value = m::min(min_value, t);
            max_value = m::max(max_value, t);
            avg_value += t;
        }
        avg_value /= f_t(dim.x*dim.y);
    }

public:
    texture2d_t(texture2d_storage_t storage,
                texture2d_config_t config,
                colour_encoding_t colour_encoding) noexcept
        : storage(std::move(storage)),
          config(config),
          config_colour_encoding(colour_encoding)
    {}
    texture2d_t(texture2d_t&&) = default;
    virtual ~texture2d_t() noexcept = default;

    /**
     * @brief Returns the byte per pixel component.
     */
    [[nodiscard]] virtual std::size_t component_bytes() const noexcept = 0;
    /**
     * @brief Returns the total size (in bytes) of the texture.
     */
    [[nodiscard]] virtual std::size_t bytes() const noexcept = 0;

    /**
     * @brief Returns the colour encoding used for the texture. Ignored for floating-point textures.
     */
    [[nodiscard]] inline colour_encoding_t colour_encoding() const noexcept { return config_colour_encoding; }

    /**
     * @brief Returns the requested texture filter.
     */
    [[nodiscard]] virtual texture_filter_type_e get_tex_filter() const noexcept = 0;

    /**
     * @brief Returns the pixel layout.
     */
    [[nodiscard]] inline const auto pixel_layout() const noexcept { return storage.layout.layout; }
    /**
     * @brief Returns the number of components per pixel.
     */
    [[nodiscard]] inline const auto components() const noexcept { return storage.layout.components; }
    /**
     * @brief Returns the image dimension of the texture.
     */
    [[nodiscard]] inline vec2u32_t dimensions() const noexcept {
        return storage.info.dim;
    }

    [[nodiscard]] inline const auto& get_storage() const noexcept { return storage; }

    /**
     * @brief Access a single texel at an image coordinate.
     */
    [[nodiscard]] virtual vec4_t texel(vec2i32_t coord) const noexcept = 0;

    /**
     * @brief Texture minimal texel value
     */
    [[nodiscard]] inline vec4_t minimum_value() const noexcept { return min_value; }

    /**
     * @brief Texture maximum texel value
     */
    [[nodiscard]] inline vec4_t maximum_value() const noexcept { return max_value; }

    /**
     * @brief Texture average texel value
     */
    [[nodiscard]] inline vec4_t mean_value() const noexcept { return avg_value; }
    
    /**
     * @brief Returns TRUE for textures that are constant.
     */
    [[nodiscard]] inline bool is_constant() const noexcept {
        return min_value == max_value;
    }

    /**
     * @brief Filters the texture.
     * @param uv ∈[0,1]² uv coordinates
     * @param dudp partial derivatives of u w.r.t. sampling area
     * @param dvdp partial derivatives of v w.r.t. sampling area
     */
    [[nodiscard]] virtual vec4_t filter(vec2_t uv, const vec2_t dudp, const vec2_t dvdp) const noexcept = 0;

    [[nodiscard]] virtual scene::element::info_t description() const = 0;

public:
    /**
     * @brief Loads a texture from a path.
     * @param colour_encoding (optional) Desired colour encoding, if not provided uses the colour encoding of the stored bitmap
     */
    static std::unique_ptr<texture2d_t> load_from_path(
            const wt_context_t& ctx,
            const std::filesystem::path& path,
            const texture2d_config_t config = {},
            std::optional<colour_encoding_t> colour_encoding = {});
};


/**
 * @brief Texture filtering implementation.
 */
template <
    texel TexelT,
    texture_filter_type_e texfilter,
    pixel_layout_e layout,
    colour_encoding_type_e colour_encoding_type
>
class texture2d_tmpl_t final : public texture2d_t {
private:
    static constexpr auto layout_data = pixel_layout_t{ layout };
    static constexpr int alpha_comp =
        layout==pixel_layout_e::LA ? 1 : 
        layout==pixel_layout_e::RGBA ? 3 : -1;
    static constexpr auto comps = layout_data.components;
    
    using vec_native_t = vec<comps, f_t>;

    [[nodiscard]] static inline auto convert_to_vec4(const vec_native_t& v) noexcept {
        if constexpr (comps==4) return v;

        vec4_t v4;
        for (int i=0;i<comps;++i) v4[i]=v[i];
        return convert_pixel_layout(layout, pixel_layout_e::RGBA, v4);
    }

    [[nodiscard]] inline colour_encoding_t internal_colour_encoding() const noexcept {
        if constexpr (colour_encoding_type == colour_encoding_type_e::gamma)
            return { colour_encoding_type, this->config_colour_encoding.gamma };
        return { colour_encoding_type };
    }

public:
    texture2d_tmpl_t(texture2d_storage_t storage,
                     texture2d_config_t config,
                     colour_encoding_t colour_encoding)
        : texture2d_t(std::move(storage), config, colour_encoding)
    {
        // sanity
        assert(layout == pixel_layout());
        assert(colour_encoding_type == this->config_colour_encoding.type());
        assert(texfilter == config.filter);

        this->compute_texture_data();
    }

    texture2d_tmpl_t(texture2d_tmpl_t&&) noexcept = default;
    texture2d_tmpl_t(const texture2d_tmpl_t& o) noexcept = default;

    [[nodiscard]] inline auto clamp_texel_native(vec_native_t v) const noexcept {
        for (int i=0;i<comps;++i) v[i] = clamp_texel(this->config.texel_clamp_mode, v[i]);
        return v;
    }

    /**
     * @brief Returns the byte per pixel component.
     */
    [[nodiscard]] std::size_t component_bytes() const noexcept override { return sizeof(TexelT); }
    /**
     * @brief Returns the total size (in bytes) of the texture.
     */
    [[nodiscard]] std::size_t bytes() const noexcept override { return storage.bytes(); }

    /**
     * @brief Returns the requested texture filter.
     */
    [[nodiscard]] texture_filter_type_e get_tex_filter() const noexcept override { return texfilter; }

    /**
     * @brief Access a single texel at an image coordinate.
     *        Returns the texel in the image's native pixel layout.
     *        Does not clamp the returned value (ignores ``config.texel_clamp_mode``).
     */
    [[nodiscard]] vec_native_t texel_native(vec2i32_t coord) const noexcept {
        const auto& dim  = dimensions();

        // apply uv wrapping configuration
        coord = { 
            wrap_coord(config.uwrap, coord.x, dim.x), 
            wrap_coord(config.vwrap, coord.y, dim.y)
        };
        // handle out-of-bounds with constant texel value
        if (coord.x<0 || coord.y<0) {
            static constexpr auto black_texel = black_value_for_bitmap_type<TexelT>();
            static constexpr auto white_texel = white_value_for_bitmap_type<TexelT>();

            f_t v;
            if (coord.x<0) v = config.uwrap==texture_wrap_mode_e::black ? black_texel : white_texel;
            else           v = config.vwrap==texture_wrap_mode_e::black ? black_texel : white_texel;

            auto ret = vec_native_t{ v };
            if constexpr (alpha_comp!=-1)
                ret[alpha_comp] = 1;
            return ret;
        }

        const auto offset = storage.texel_offset((vec2u32_t)coord);
        const auto texel  = ((const vec<comps,TexelT>*)storage.data.get())[offset];

        vec_native_t texelfp;
        if constexpr (!std::is_floating_point_v<TexelT>) {
            for (int c=0;c<comps;++c) {
            // colour encoding (for unorm/snorm textures): convert to linear floating point
            // alpha assumed to be linear
            texelfp[c] = c==alpha_comp ?
                internal_colour_encoding().to_fp(texel[c]) :
                internal_colour_encoding().to_linear_fp(texel[c]);
            }
        } else {
            texelfp = vec_native_t{ texel };
        }

        return texelfp;
    }

    /**
     * @brief Access a single texel at an image coordinate. Returns the texel in RGBA layout.
     *        Does not clamp the returned value (ignores ``config.texel_clamp_mode``).
     */
    [[nodiscard]] vec4_t texel(vec2i32_t coord) const noexcept override {
        return convert_to_vec4(texel_native(coord));
    }

    /**
     * @brief Filter using bilinear interpolation.
     */
    [[nodiscard]] inline vec_native_t bilinear_native(vec2_t uv) const noexcept {
        const auto& dim = dimensions();

        uv = vec2_t{ dim }*uv - vec2_t{ .5 };
        const auto iuv = vec2i32_t{ floor(uv) };
        const auto f   = m::fract(uv);

        const auto t00 = texel_native(iuv + vec2i32_t{ 0,0 });
        const auto t01 = texel_native(iuv + vec2i32_t{ 0,1 });
        const auto t10 = texel_native(iuv + vec2i32_t{ 1,0 });
        const auto t11 = texel_native(iuv + vec2i32_t{ 1,1 });

        const auto ret = m::mix(
            m::mix(t00, t10, f.x),
            m::mix(t01, t11, f.x),
            f.y
        );
        assert(m::isfinite(ret));
        return ret;
    }

    /**
     * @brief Filter using bilinear interpolation.
     */
    [[nodiscard]] inline vec4_t bilinear(vec2_t uv) const noexcept {
        return convert_to_vec4(bilinear_native(uv));
    }

    /**
     * @brief Filter using bicubic interpolation.
     */
    [[nodiscard]] inline vec_native_t bicubic_native(vec2_t uv) const noexcept {
        const auto& dim = dimensions();

        uv = vec2_t{ dim }*uv - vec2_t{ .5 };
        const auto iuv = vec2i32_t{ floor(uv) };
        const auto f   = m::fract(uv);

        constexpr auto filter = [](f_t x, auto p0,auto p1,auto p2,auto p3) noexcept {
            return
                                p1 + 
                f_t(.5)*x     * (-p0 + p2) +
                f_t(.5)*x*x   * (f_t(2.)*p0 - f_t(5.)*p1 + f_t(4.)*p2 - p3) +
                f_t(.5)*x*x*x * (-p0 + f_t(3.)*p1 - f_t(3.)*p2 + p3);
        };

        vec_native_t ts[4];
        for (int y=0;y<4;++y) {
            const auto p0 = texel_native(iuv + vec2i32_t{ -1,y-1 });
            const auto p1 = texel_native(iuv + vec2i32_t{  0,y-1 });
            const auto p2 = texel_native(iuv + vec2i32_t{  1,y-1 });
            const auto p3 = texel_native(iuv + vec2i32_t{  2,y-1 });
            ts[y] = filter(f.x, p0,p1,p2,p3);
        }

        const auto ret = filter(f.y, ts[0],ts[1],ts[2],ts[3]);
        assert(m::isfinite(ret));
        return ret;
    }

    /**
     * @brief Filter using bicubic interpolation.
     */
    [[nodiscard]] inline vec4_t bicubic(vec2_t uv) const noexcept {
        return convert_to_vec4(bicubic_native(uv));
    }

    /**
     * @brief Filters the texture.
     * @param uv ∈[0,1]² uv coordinates
     * @param dudp partial derivatives of u w.r.t. sampling area
     * @param dvdp partial derivatives of v w.r.t. sampling area
     */
    [[nodiscard]] vec_native_t filter_native(vec2_t uv,
                                             const vec2_t dudp,
                                             const vec2_t dvdp) const noexcept {
        std::chrono::high_resolution_clock::time_point start;
        if constexpr (bitmap_stats::additional_bitmap_counters)
            start = std::chrono::high_resolution_clock::now();

        // flip v
        uv.y = 1-uv.y;

        if constexpr (texfilter == texture_filter_type_e::nearest) {
            // nearest texel
            const auto& dim = dimensions();
            const auto coords = vec2i32_t{ m::round(vec2_t{ dim }*uv - vec2_t{ .5 }) };
            auto ret = texel_native(coords);

            bitmap_stats::on_bitmap_filter(1, start);   // collect stats
            return clamp_texel_native(ret);
        }
        if constexpr (texfilter == texture_filter_type_e::bilinear) {
            // bilinear interpolation
            auto ret = bilinear_native(uv);

            bitmap_stats::on_bitmap_filter(4, start);   // collect stats
            return clamp_texel_native(ret);
        }
        if constexpr (texfilter == texture_filter_type_e::bicubic) {
            // bicubic interpolation
            auto ret = bicubic_native(uv);

            bitmap_stats::on_bitmap_filter(16, start);   // collect stats
            return clamp_texel_native(ret);
        }

        unreachable();
    }

    /**
     * @brief Filters the texture.
     * @param uv ∈[0,1]² uv coordinates
     * @param dudp partial derivatives of u w.r.t. sampling area
     * @param dvdp partial derivatives of v w.r.t. sampling area
     */
    [[nodiscard]] vec4_t filter(vec2_t uv, const vec2_t dudp, const vec2_t dvdp) const noexcept override {
        return convert_to_vec4(filter_native(uv, dudp, dvdp));
    }

    [[nodiscard]] scene::element::info_t description() const override {
        using namespace scene::element;
        return scene::element::info_t{
            .cls  = "texel storage",
            .type = "texture2d",
            .attribs = {
                { "size",   attributes::make_vector(dimensions()) },
                { "layout", attributes::make_enum(storage.layout.layout) },
                { "filter", attributes::make_enum(texfilter) }
            }
        };
    }
};

}
