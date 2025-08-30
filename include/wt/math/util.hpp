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
#include <optional>

#include <wt/math/type_traits.hpp>

#include <wt/math/common.hpp>
#include <wt/math/eft/eft.hpp>

#include <wt/math/simd/wide_vector.hpp>

namespace wt::util {

/**
 * @brief Returns TRUE if the point 'p' lies within the circle centred at 'o' with radius 'r'
 */
[[nodiscard]] inline bool is_point_in_circle(const vec2_t& p, 
                                             const f_t r, const vec2_t& o = { 0,0 }) noexcept {
    return m::length2(p-o)<=m::sqr(r);
}

/**
 * @brief Returns TRUE if the point 'p' lies within a (2/3/4-dimensional) sphere centred at 'o' with radius 'r'
 */
template <QuantityVectorOf<isq::length> Vp, QuantityVectorOf<isq::length> Vo>
[[nodiscard]] inline bool is_point_in_sphere(const Vp& p, 
                                             const Length auto r,
                                             const Vo& o) noexcept
    requires (element_count_v<Vp> == element_count_v<Vo>)
{
    return m::length2(p-o)<=m::sqr(r);
}

/**
 * @brief Returns TRUE if the point 'p' lies within the ellipse centred at 'o', with semi-major and semi-minor axes 'rx' and 'ry'.
 */
[[nodiscard]] inline bool is_point_in_ellipse(const vec2_t& p, 
                                              const f_t rx, const f_t ry, const vec2_t& o = { 0,0 }) noexcept {
    const auto r = (p-o)/vec2_t{ rx,ry };
    return m::dot(r,r) <= 1;
}

/**
 * @brief Returns TRUE if the point 'p' lies within the (axis-aligned) ellipsoid centred at 'o', with axes 'r'.
 */
template <QuantityVectorOf<isq::length> Vp, QuantityVectorOf<isq::length> Vr, QuantityVectorOf<isq::length> Vo>
[[nodiscard]] inline bool is_point_in_ellipsoid(const Vp& p, 
                                                const Vr& r, 
                                                const Vo& o) noexcept
    requires (element_count_v<Vp> == element_count_v<Vo>) && (element_count_v<Vp> == element_count_v<Vr>)
{
    const auto q = (p-o)/r;
    return m::dot(q,q) <= 1;
}

/**
 * @brief Returns TRUE if the point 'p' lies within the triangle abc
 */
[[nodiscard]] inline bool is_point_in_triangle(const vec2_t& p, 
                                               const vec2_t& a, const vec2_t& b, const vec2_t& c) noexcept {
    constexpr auto sgn = [](auto p1, auto p2, auto p3) {
        return m::eft::diff_prod(p1.x-p3.x, p2.y-p3.y, p2.x-p3.x, p1.y-p3.y);
    };

    const auto s1 = sgn(p, a, b);
    const auto s2 = sgn(p, b, c);
    const auto s3 = sgn(p, c, a);

    const auto neg = s1<0 || s2<0 || s3<0;
    const auto pos = s1>0 || s2>0 || s3>0;
    return !(neg && pos);
}

/**
 * @brief Returns TRUE if the point 'p' lies within the triangle abc.
          'p' is assumed to lie in the triangle plane, and points 'a','b','c' are assumed to NOT be co-linear.
 */
[[nodiscard]] inline bool is_point_in_triangle(const vec3_t& p, 
                                               const vec3_t& a, const vec3_t& b, const vec3_t& c) noexcept {
    // compute barycentrics
    const auto v0 = b-a,
               v1 = c-a;
    const auto u = p-a;
    const auto d00 = m::dot(v0, v0);
    const auto d01 = m::dot(v0, v1);
    const auto d11 = m::dot(v1, v1);
    const auto d20 = m::dot(u, v0);
    const auto d21 = m::dot(u, v1);

    const auto d = m::eft::diff_prod(d00,d11, d01,d01);
    const f_t sgn = d>0 ? 1 : -1;

    const auto alpha = m::eft::diff_prod(d11,d20, d01,d21);
    const auto beta  = m::eft::diff_prod(d00,d21, d01,d20);

    return sgn*alpha>=0 && sgn*beta>=0 && sgn*(alpha+beta)<=sgn*d;
}

/**
 * @brief Returns TRUE if the point 'p' lies within the triangle abc
 */
template <QuantityVectorOf<isq::length> Vp, 
          QuantityVectorOf<isq::length> Va, QuantityVectorOf<isq::length> Vb, QuantityVectorOf<isq::length> Vc> 
[[nodiscard]] inline bool is_point_in_triangle(const Vp& p, 
                                               const Va& a, 
                                               const Vb& b, 
                                               const Vc& c) noexcept
{
    return is_point_in_triangle(u::to_m(p), u::to_m(a), u::to_m(b), u::to_m(c));
}

/**
 * @brief Returns TRUE if the point 'p' lies within the triangle abc. Wide test.
          'p' is assumed to lie in the triangle plane, and points 'a','b','c' are assumed to NOT be co-linear.
 */
template <std::size_t W>
[[nodiscard]] inline b_w_t<W> is_point_in_triangle(
        const pqvec3_w_t<W>& p,
        const pqvec3_w_t<W>& a,
        const pqvec3_w_t<W>& b,
        const pqvec3_w_t<W>& c) noexcept {
    const auto v0 = b-a;
    const auto v1 = c-a;
    const auto u  = p-a;

    const auto d00 = m::dot(v0, v0);
    const auto d01 = m::dot(v0, v1);
    const auto d11 = m::dot(v1, v1);
    const auto d20 = m::dot(u, v0);
    const auto d21 = m::dot(u, v1);

    const auto d = m::eft::diff_prod(d00,d11, d01,d01);
    const auto alpha = m::eft::diff_prod(d11,d20, d01,d21);
    const auto beta  = m::eft::diff_prod(d00,d21, d01,d20);

    const auto mask = d > d.zero();
    // d>0 ? 1 : -1
    const auto sgn  = m::selectv(f_w_t<W>{ f_t(-1) }, f_w_t<W>{ f_t(1) }, mask);
    const auto sa = alpha * sgn;
    const auto sb = beta  * sgn;

    const auto cond1 = sa >= sa.zero();
    const auto cond2 = sb >= sb.zero();
    const auto cond3 = (sa+sb) <= sgn*d;

    return cond1 && cond2 && cond3;
}

/**
 * @brief Returns TRUE if the point 'p' lies within the rectangle abcd
 */
[[nodiscard]] inline bool is_point_in_rectangle(
        const vec2_t& p, 
        const vec2_t& a, const vec2_t& b, const vec2_t& c, const vec2_t& d) noexcept {
    const auto d1 = m::dot(b-a,p-a);
    const auto d2 = m::dot(d-a,p-a);
    const auto ab2 = m::dot(b-a,b-a);
    const auto ad2 = m::dot(d-a,d-a);
    return d1>=0 && d2>=0 && d1<=ab2 && d2<=ad2;
}

/**
 * @brief Returns TRUE if the point 'p' lies within the rectangle abcd
 */
template <QuantityVectorOf<isq::length> Vp, 
          QuantityVectorOf<isq::length> Va, QuantityVectorOf<isq::length> Vb, QuantityVectorOf<isq::length> Vc, QuantityVectorOf<isq::length> Vd> 
[[nodiscard]] inline bool is_point_in_rectangle(
        const Vp& p, 
        const Va& a, 
        const Vb& b, 
        const Vc& c, 
        const Vd& d) noexcept
    requires (element_count_v<Va> == element_count_v<Vp>) && (element_count_v<Vb> == element_count_v<Vp>) && (element_count_v<Vc> == element_count_v<Vp>) && (element_count_v<Vd> == element_count_v<Vp>)
{
    return is_point_in_rectangle(u::to_m(p), u::to_m(a), u::to_m(b), u::to_m(c), u::to_m(d));
}


template <QuantityVectorOf<isq::length> Va, QuantityVectorOf<isq::length> Vb, QuantityVectorOf<isq::length> Vc>
[[nodiscard]] inline auto tri_surface_area(const Va& a, 
                                           const Vb& b, 
                                           const Vc& c) noexcept
    requires (element_count_v<Va> == 3) && (element_count_v<Vb> == 3) && (element_count_v<Vc> == 3)
{
    return f_t(.5) * m::length(m::cross(c-a,b-a));
}

template <QuantityVectorOf<isq::length> Va, QuantityVectorOf<isq::length> Vb, QuantityVectorOf<isq::length> Vc>
[[nodiscard]] inline auto tri_face_normal(
        const Va& a, 
        const Vb& b, 
        const Vc& c) noexcept
    requires (element_count_v<Va> == 3) && (element_count_v<Vb> == 3) && (element_count_v<Vc> == 3)
{
    const auto u = b-a;
    const auto v = c-a;

    const auto n = m::cross(u,v);

    using R = std::optional<decltype(m::normalize(n))>;

    if (!m::all(m::iszero(n))) 
        return std::make_optional(m::normalize(n));
    return R{};
}

}
