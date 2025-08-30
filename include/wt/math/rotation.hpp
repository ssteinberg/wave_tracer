/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/common.hpp>
#include <wt/math/eft/eft.hpp>

namespace wt::util {

/**
 * @brief Builds a 3d rotation matrix that rotates around an axis ``d`` by angle ``angle``.
 */
inline auto rotation_matrix(const dir3_t& d, const Angle auto angle) noexcept {
    const auto c = m::cos(angle);
    const auto s = m::sin(angle);

    const auto x = d.x, y = d.y, z = d.z;
    const auto U = m::outer(d,d);
    
    return m::transpose(U*(1-c) + mat3_t(
         c,   -z*s,  y*s,
         z*s,  c,   -x*s,
        -y*s,  x*s,  c
    ));
}

/**
 * @brief Builds a 3d rotation matrix that rotates from a given unit vector 'from' to a given unit vector 'to'.
 *        Adapted from PBR book.
 */
inline auto rotation_matrix(const dir3_t& from, const dir3_t& to) noexcept {
    static constexpr f_t threshold = .72;
    vec3_t axis;
    if (m::abs(from.x)<threshold && m::abs(to.x)<threshold)
        axis = { 1,0,0 };
    else if (m::abs(from.y)<threshold && m::abs(to.y)<threshold)
        axis = { 0,1,0 };
    else
        axis = { 0,0,1 };

    const auto u = axis - vec3_t{ from }, v = axis - vec3_t{ to };
    const auto recpu = 1/m::dot(u,u);
    const auto recpv = 1/m::dot(v,v);

    mat3_t R{};
    for (int i=0; i<3; ++i)
    for (int j=0; j<3; ++j)
        R[j][i] = ((i==j) ? 1 : 0) -
                    2 * recpu * u[i] * u[j] -
                    2 * recpv * v[i] * v[j] +
                    4 * m::dot(u,v) * (recpu*recpv) * v[i] * u[j];

    return R;
}

/**
 * @brief Builds a 2d rotation matrix that rotates from a given unit vector 'from' to a given unit vector 'to'.
 */
inline auto rotation_matrix(const dir2_t& from, const dir2_t& to) noexcept {
    const auto xa = from.x, xb = to.x;
    const auto ya = from.y, yb = to.y;

    const auto X = m::eft::sum_prod(xa,xb, ya,yb);
    return mat2_t{
        X,
        m::eft::diff_prod(xa,yb, xb,ya),
        m::eft::diff_prod(xb,ya, xa,yb),
        X
    };
}

}

