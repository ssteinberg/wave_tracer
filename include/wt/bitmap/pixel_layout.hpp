/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cstdint>
#include <cassert>

#include <wt/util/unreachable.hpp>

#include <wt/math/common.hpp>
#include <wt/spectrum/colourspace/RGB/RGB.hpp>

namespace wt::bitmap {

enum class pixel_layout_e : std::int8_t {
    L,
    LA,
    RGB,
    RGBA,
    Custom,
};

struct pixel_layout_t {
    inline constexpr auto components_for_layout(pixel_layout_e layout) noexcept {
        switch (layout) {
        case pixel_layout_e::L:     return 1;
        case pixel_layout_e::LA:    return 2;
        case pixel_layout_e::RGB:   return 3;
        case pixel_layout_e::RGBA:  return 4;
        default:                    return 0;
        }
        unreachable();
    }

    pixel_layout_e layout;
    std::uint8_t   components=0;

    constexpr pixel_layout_t() noexcept = default;
    constexpr pixel_layout_t(pixel_layout_e layout) noexcept 
        : layout(layout), components(components_for_layout(layout))
    {
        assert(components>0);
    }
    explicit constexpr pixel_layout_t(std::uint8_t components) noexcept 
        : layout(pixel_layout_e::Custom), components(components)
    {
        assert(components>0);
    }

    pixel_layout_t(const pixel_layout_t&) noexcept = default;
    pixel_layout_t(pixel_layout_t&&) noexcept = default;
    pixel_layout_t& operator=(const pixel_layout_t&) noexcept = default;
    pixel_layout_t& operator=(pixel_layout_t&&) noexcept = default;

    [[nodiscard]] inline std::strong_ordering operator<=>(const pixel_layout_t& o) const noexcept = default;
};

/**
 * @brief Converts between different pixel layouts. Custom is unsupported.
 */
constexpr inline vec4_t convert_pixel_layout(pixel_layout_e from, pixel_layout_e to, vec4_t texels) noexcept {
    if (from==pixel_layout_e::Custom || to==pixel_layout_e::Custom) {
        assert(false);
        return {};
    }

    // src->RGBA
    if (from==pixel_layout_e::L || from==pixel_layout_e::LA) {
        texels.w = from==pixel_layout_e::LA ? texels.y : 1;
        texels.y = texels.z = texels.x;
    }
    else if (from!=pixel_layout_e::RGBA)
        texels.w = 1;

    // RGBA->dst
    if (to==pixel_layout_e::L || to==pixel_layout_e::LA) {
        const auto L = colourspace::luminance(vec3_t{ texels });
        const auto a = to==pixel_layout_e::LA ? texels.w : 0;
        return { L,a,0,0 };
    }
    if (to!=pixel_layout_e::RGBA)
        texels.w = 0;
    return texels;
}

}
