/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/util/unreachable.hpp>
#include <wt/math/common.hpp>
#include <wt/spectrum/colourspace/whitepoint.hpp>

namespace wt::colourspace {

/**
 * @brief Colourspaces for RGB tristimulus colour representation.
 *\rst
 * .. image:: /_static/code/RGBgamut.png
 *     :scale: 20%
 *     :alt: RGB gamuts
 *\endrst
 */
enum class RGB_colourspace_e : std::uint8_t {
    CIE,
    sRGB,
    Adobe1998,
    AdobeWideGamut,
    ProPhoto,
    DCI_P3,
    rec2020,
};

/**
 * @brief Returns a pair with the reference white point for the colourspace and the XYZ-to-RGB conversion matrix.
 */
inline auto XYZ_to_RGB_for_colourspace(RGB_colourspace_e colourspace) noexcept {
    switch (colourspace) {
    case RGB_colourspace_e::CIE:
        return std::make_pair(
            white_point_e::E,
            m::transpose(mat3_t{
                 2.3706743, -0.9000405, -0.4706338,
                -0.5138850,  1.4253036,  0.0885814,
                 0.0052982, -0.0146949,  1.0093968,
            })
        );
    case RGB_colourspace_e::sRGB:
        return std::make_pair(
            white_point_e::D65,
            m::transpose(mat3_t{
                 3.2404542, -1.5371385, -0.4985314,
                -0.9692660,  1.8760108,  0.0415560,
                 0.0556434, -0.2040259,  1.0572252,
            })
        );
    case RGB_colourspace_e::Adobe1998:
        return std::make_pair(
            white_point_e::D65,
            m::transpose(mat3_t{
                 2.0413690, -0.5649464, -0.3446944,
                -0.9692660,  1.8760108,  0.0415560,
                 0.0134474, -0.1183897,  1.0154096,
            })
        );
    case RGB_colourspace_e::AdobeWideGamut:
        return std::make_pair(
            white_point_e::D50,
            m::transpose(mat3_t{
                 1.4628067, -0.1840623, -0.2743606,
                -0.5217933,  1.4472381,  0.0677227,
                 0.0349342, -0.0968930,  1.2884099,
            })
        );
    case RGB_colourspace_e::ProPhoto:
        return std::make_pair(
            white_point_e::D50,
            m::transpose(mat3_t{
                 1.3459433, -0.2556075, -0.0511118,
                -0.5445989,  1.5081673,  0.0205351,
                 0.0000000,  0.0000000,  1.2118128,
            })
        );
    case RGB_colourspace_e::DCI_P3:
        return std::make_pair(
            white_point_e::D65,
            m::transpose(mat3_t{
                 2.4934969, -0.9313836, -0.4027108,
                -0.8294890,  1.7626641,  0.0236247,
                 0.0358458, -0.0761724,  0.9568845,
            })
        );
    case RGB_colourspace_e::rec2020:
        return std::make_pair(
            white_point_e::D65,
            m::transpose(mat3_t{
                 0.6369580, 0.1446169, 0.1688810,
                 0.2627002, 0.6779981, 0.0593017,
                 0.0000000, 0.0280727, 1.0609851,
            })
        );
    }
    unreachable();
}

/**
 * @brief Returns a pair with the reference white point for the colourspace and the RGB-to-XYZ conversion matrix.
 */
inline auto RGB_to_XYZ_for_colourspace(RGB_colourspace_e colourspace) noexcept {
    auto conv = XYZ_to_RGB_for_colourspace(colourspace);
    conv.second = m::inverse(conv.second);
    return conv;
}

inline auto conversion_matrix_XYZ_to_RGB(RGB_colourspace_e colourspace,
                                         white_point_e whitepoint) noexcept {
    // XYZ-to-RGB conversion matrix
    const auto conv = XYZ_to_RGB_for_colourspace(colourspace);
    auto M = conv.second;
    // source and destination white points
    const auto swXYZ = reference_XYZ_for_white_point(conv.first);
    const auto dwXYZ = reference_XYZ_for_white_point(whitepoint);

    // transform white point
    if (swXYZ != dwXYZ) {
        M *= chromatic_adaptation_transform(swXYZ, dwXYZ);
    }

    return M;
}

inline auto conversion_matrix_RGB_to_XYZ(RGB_colourspace_e colourspace,
                                         white_point_e whitepoint) noexcept {
    // RGB-to-XYZ conversion matrix
    const auto conv = RGB_to_XYZ_for_colourspace(colourspace);
    auto M = conv.second;
    // source and destination white points
    const auto swXYZ = reference_XYZ_for_white_point(conv.first);
    const auto dwXYZ = reference_XYZ_for_white_point(whitepoint);

    // transform white point
    if (swXYZ != dwXYZ) {
        M *= chromatic_adaptation_transform(swXYZ, dwXYZ);
    }

    return M;
}


/**
 * @brief BT.709 luminance for a (linear) RGB triplet
 */
inline auto luminance(const vec3_t& rgb) noexcept {
    return m::max<f_t>(0, m::dot(vec3_t{ .2126,.7152,.0722 }, rgb));
}

/**
 * @brief Converts XYZ to CIE LAB for a desired white point
 */
inline auto XYZ_to_LAB(const vec3_t& XYZ, white_point_e whitepoint) noexcept {
    constexpr auto f = [](const f_t t) {
        constexpr auto d = f_t(6./29.);
        constexpr auto d3 = d*d*d;

        return t>d3 ?
            m::pow<f_t>(t, f_t(1./3.)) :
            t * f_t(1./(3.*d*d)) + f_t(4./29.);
    };

    const auto& wp = reference_XYZ_for_white_point(whitepoint);
    const auto Xn = wp.x;
    const auto Yn = wp.y;
    const auto Zn = wp.z;

    const auto fX = f(XYZ.x / Xn);
    const auto fY = f(XYZ.y / Yn);
    const auto fZ = f(XYZ.z / Zn);

    const auto Lstar = f_t(1.16) * fY - f_t(.16);
    const auto a = f_t(5.) * (fX - fY);
    const auto b = f_t(2.) * (fY - fZ);

    return vec3_t{ Lstar,a,b };
}


namespace sRGB {

/**
 * @brief gamma correction: sRGB to linear colourspace
 */
inline auto from_linear(const f_t x) noexcept {
    if (x<=f_t(.0031308))
        return m::max<f_t>(0, f_t(12.92)*x);
    return f_t(1.055) * m::pow(x, f_t(1./2.4)) - f_t(.055);
}
/**
 * @brief gamma correction: sRGB to linear colourspace
 */
inline auto from_linear(const vec3_t& rgb) noexcept {
    return vec3_t{
        from_linear(rgb.x),
        from_linear(rgb.y),
        from_linear(rgb.z),
    };
}

/**
 * @brief gamma correction: linear to sRGB colourspace
 */
inline auto to_linear(const f_t x) noexcept {
    if (x<=f_t(.04045))
        return m::max<f_t>(0, x/f_t(12.92));
    return m::pow((x + f_t(.055))/f_t(1.055), f_t(2.4));
}
/**
 * @brief gamma correction: linear to sRGB colourspace
 */
inline auto to_linear(const vec3_t& rgb) noexcept {
    return vec3_t{
        to_linear(rgb.x),
        to_linear(rgb.y),
        to_linear(rgb.z),
    };
}

}

}
