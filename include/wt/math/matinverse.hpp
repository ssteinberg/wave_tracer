/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/defs.hpp>
#include <wt/math/common.hpp>
#include <wt/math/eft/eft.hpp>

namespace wt::m {

template <typename T>
constexpr inline auto inverse(const mat2<T>& m) noexcept {
    const auto d = determinant(m);
    const auto recp_d = T(1) / d;

    return mat2<T>(
        +m[1][1] * recp_d,
        -m[0][1] * recp_d,
        -m[1][0] * recp_d,
        +m[0][0] * recp_d);
}

template <typename T>
constexpr inline auto inverse(const mat3<T>& m) noexcept {
    // from glm::inverse

    mat3<T> inv;
    inv[0][0] = +eft::diff_prod(m[1][1],m[2][2], m[2][1],m[1][2]);
    inv[1][0] = -eft::diff_prod(m[1][0],m[2][2], m[2][0],m[1][2]);
    inv[2][0] = +eft::diff_prod(m[1][0],m[2][1], m[2][0],m[1][1]);
    inv[0][1] = -eft::diff_prod(m[0][1],m[2][2], m[2][1],m[0][2]);
    inv[1][1] = +eft::diff_prod(m[0][0],m[2][2], m[2][0],m[0][2]);
    inv[2][1] = -eft::diff_prod(m[0][0],m[2][1], m[2][0],m[0][1]);
    inv[0][2] = +eft::diff_prod(m[0][1],m[1][2], m[1][1],m[0][2]);
    inv[1][2] = -eft::diff_prod(m[0][0],m[1][2], m[1][0],m[0][2]);
    inv[2][2] = +eft::diff_prod(m[0][0],m[1][1], m[1][0],m[0][1]);

    const auto d = determinant(m);
    inv *= vec3<T>{ T(1)/d };
    return inv;
}

template <typename T>
constexpr inline auto inverse(const mat4<T>& m) noexcept {
    // TODO: can do better, but this is rarely used.
    return glm::inverse(m);
}

}
