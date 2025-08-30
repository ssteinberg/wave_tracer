/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <utility>
#include <wt/util/unreachable.hpp>
#include <wt/math/common.hpp>

namespace wt::colourspace {

enum class white_point_e : std::uint8_t {
    D50,
    D55,
    D65,
    D75,
    E,
    DCI
};

/**
 * @brief Returns the XYZ tristimulus reference white point
 */
inline auto reference_XYZ_for_white_point(white_point_e whitepoint) noexcept {
    switch(whitepoint) {
    case white_point_e::D50:  return vec3_t{ 0.96422, 1.00000, 0.82521 };
    case white_point_e::D55:  return vec3_t{ 0.95682, 1.00000, 0.92149 };
    case white_point_e::D65:  return vec3_t{ 0.95047, 1.00000, 1.08883 };
    case white_point_e::D75:  return vec3_t{ 0.94972, 1.00000, 1.22638 };
    case white_point_e::E:    return vec3_t{ 1.00000, 1.00000, 1.00000 };
    case white_point_e::DCI:  return vec3_t{ 0.95046, 1.00000, 1.08906 };
    }
    unreachable();
}

/**
 * @brief Bradford chromatic adaptation transformation in XYZ colourspace.
 */
inline auto chromatic_adaptation_transform(const vec3_t& src_white_XYZ, const vec3_t& dest_white_XYZ) noexcept {
    const auto MA = m::transpose(mat3_t{
         0.8951000,  0.2664000, -0.1614000,
        -0.7502000,  1.7135000,  0.0367000,
         0.0389000, -0.0685000,  1.0296000,
    });
    const auto invMA = m::transpose(mat3_t{
         0.9869929, -0.1470543,  0.1599627,
         0.4323053,  0.5183603,  0.0492912,
        -0.0085287,  0.0400428,  0.9684867,
    });

    const auto rho_S = MA * src_white_XYZ;
    const auto rho_D = MA * dest_white_XYZ;

    return invMA * m::diagonal_mat(rho_D/rho_S) * MA;
}

}
