/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <memory>

#include <cassert>

#include <utility>
#include <concepts>

#include <wt/util/unreachable.hpp>

#include <wt/math/common.hpp>
#include <wt/bitmap/bitmap.hpp>
#include <wt/bitmap/texel_convert.hpp>
#include <wt/bitmap/common.hpp>

#include <wt/spectrum/colourspace/RGB/RGB.hpp>

namespace wt::bitmap {

/** @brief 2D texture filtering mode.
 *         * ``nearest``: nearest texel.
 *         * ``bilinear``: bi-linear \f$ 2\times 2 \f$ filter.
 *         * ``bicubic`` (default): bi-cubic \f$ 4\times 4 \f$ filter.
 */
enum class texture_filter_type_e : std::uint8_t {
    nearest,
    bilinear,
    bicubic,
};

/** @brief Wrap mode (per dimension) for UV coordinates that fall outside the fundamental \f$ [0,1] \f$ range:
 *         * ``black``: produces black texels.
 *         * ``white``: produces white texels.
 *         * ``clamp``: clamps the UV coordinates to \f$ [0,1] \f$.
 *         * ``repeat`` (default): tiles the texture by using the fractional part of the UV coordinates, i.e. \f$ \vec{uv}=(-0.2,2.6) \f$ becomes \f$ \vec{uv}=(0.8,0.6) \f$.
 *         * ``mirror``: tiles the texture similar to ``repeat`` but flips every consecutive tile, i.e. \f$ \vec{uv}=(-0.2,2.6) \f$ becomes \f$ \vec{uv}=(0.2,0.6) \f$.
 */
enum class texture_wrap_mode_e : std::uint8_t {
    black,
    white,
    clamp,
    repeat,
    mirror,
};

/** @brief Clamping mode for filtered texel values:
 *         * ``none``: no clamping.
 *         * ``clamp_non_negative`` (default): clamp values to be non-negative.
 *         * ``clamp_non_positive``: clamp values to be non-positive.
 *         (Note: some texture filtering methods, like ``bicubic``, might produce negative texels even when the entire bitmap is positive).
 */
enum class texel_clamp_mode_e : std::uint8_t {
    none,
    clamp_non_negative,
    clamp_non_positive,
};


/**
 * @brief Texture filtering configuration.
 */
struct texture2d_config_t {
    texture_filter_type_e filter = texture_filter_type_e::bicubic;
    texel_clamp_mode_e texel_clamp_mode = texel_clamp_mode_e::clamp_non_negative;

    texture_wrap_mode_e uwrap = texture_wrap_mode_e::repeat;
    texture_wrap_mode_e vwrap = texture_wrap_mode_e::repeat;
};

[[nodiscard]] constexpr inline int wrap_coord(texture_wrap_mode_e wrap, int coord, std::size_t dim) noexcept {
    if (coord>=0 && coord<dim)
        return coord;

    switch (wrap) {
    case texture_wrap_mode_e::black:
    case texture_wrap_mode_e::white:
        return coord>=dim || coord<0 ? -1 : coord;
    case texture_wrap_mode_e::clamp:
        return m::clamp(coord, 0, m::max<int>(1,dim)-1);
    case texture_wrap_mode_e::repeat:
        return m::modulo<int>(coord, dim);
    case texture_wrap_mode_e::mirror:
        coord = m::modulo<int>(coord, 2*dim);
        return coord>=dim ? 2*dim-1-coord : coord;
    }
    unreachable();
}

template <FloatingPoint T>
[[nodiscard]] constexpr inline auto clamp_texel(texel_clamp_mode_e mode, T texel) noexcept {
    switch (mode) {
    case texel_clamp_mode_e::none: return texel;
    case texel_clamp_mode_e::clamp_non_negative: return m::max(T(0),texel);
    case texel_clamp_mode_e::clamp_non_positive: return m::min(T(0),texel);
    }
    unreachable();
}

template <std::integral T>
[[nodiscard]] constexpr inline T black_value_for_bitmap_type() noexcept {
    return limits<T>::min();
}
template <std::integral T>
[[nodiscard]] constexpr inline T white_value_for_bitmap_type() noexcept {
    return limits<T>::max();
}
template <FloatingPoint T>
[[nodiscard]] constexpr inline T black_value_for_bitmap_type() noexcept {
    return T(0);
}
template <FloatingPoint T>
[[nodiscard]] constexpr inline T white_value_for_bitmap_type() noexcept {
    return T(1);
}


/**
 * @brief Generic storage for a texture; uses an aligned, tiled memory layout.
 */
struct texture2d_storage_t {
    struct alignas(64) aligned_block_t {};
    struct tex_info_t {
        /** @brief Bitmap dimensions. */
        vec2u32_t dim;
        /** @brief Tile dimensions. */
        vec2u32_t tiles;
    };

    static constexpr vec2u32_t tile_size = { 8,4 };
    static constexpr std::uint32_t tile_texels = tile_size.x*tile_size.y;

    std::unique_ptr<aligned_block_t[]> data;
    tex_info_t info;

    /** @brief Size in bytes of a single texel component. */
    std::uint8_t comp_size;
    /** @brief Pixel layout a texel. */
    pixel_layout_t layout;

    /**
     * @brief Size in bytes of this storage.
     */
    [[nodiscard]] inline std::size_t bytes() const noexcept {
        const auto b = info.tiles.x*info.tiles.y*tile_texels * layout.components*comp_size;
        constexpr auto alignment = sizeof(aligned_block_t);
        return (b + alignment-1)/alignment * alignment;
    }

    /**
     * @brief Offset into ``data`` to texel at integer coordinates ``coords``.
     */
    [[nodiscard]] inline auto texel_offset(vec2u32_t coords) const noexcept {
        const auto tile = coords / tile_size;
        const auto p    = coords % tile_size;

        return (tile.y * info.tiles.x + tile.x) * tile_texels + p.y * tile_size.x + p.x;
    }

    /**
     * @brief Creates a bitmap from this texture storage.
     * @tparam T texel type of bitmap; must fulfil ``sizeof(T)==comp_size``.
     */
    template <texel T>
    [[nodiscard]] inline auto create_bitmap() const noexcept {
        auto bm = bitmap2d_t<T>::create(info.dim.x, info.dim.y, layout);
        if (comp_size != sizeof(T)) {
            assert(false);
            return bm;
        }

        const auto* lbuf = (const T*)data.get();
        auto* dst = bm.data();
        const auto comps = layout.components;
        for (auto y=0u;y<bm.height();++y)
        for (auto x=0u;x<bm.width();++x,dst+=comps)
            std::copy_n(lbuf + texel_offset({ x,y })*comps, comps, dst);

        return bm;
    }
};


}
