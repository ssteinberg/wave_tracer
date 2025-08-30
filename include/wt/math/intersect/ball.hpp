/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/intersect/intersect_defs.hpp>

#include <wt/math/common.hpp>
#include <wt/math/util.hpp>
#include <wt/math/simd/wide_vector.hpp>

#include <wt/math/shapes/ball.hpp>
#include <wt/math/shapes/aabb.hpp>

namespace wt::intersect {

/**
 * @brief Ball-triangle intersection test.
 *        A ball is not its spherical shell: returns TRUE for triangles contained in the ball.
 */
inline bool test_ball_tri(const ball_t& ball,
                          const pqvec3_t& a,
                          const pqvec3_t& b,
                          const pqvec3_t& c,
                          const dir3_t& n) noexcept {
    if (ball.contains(a) || ball.contains(b) || ball.contains(c))
        return true;

    // project centre upon triangle
    const auto p = ball.centre - m::dot(n,ball.centre-a)*n;
    if (util::is_point_in_triangle(p, a,b,c))
        return ball.contains(p);

    return false;
}

/**
 * @brief Ball-triangle intersection test. Wide test.
 *        A ball is not its spherical shell: returns TRUE for triangles contained in the ball.
 */
template <std::size_t W>
inline auto test_ball_tri(
        const ball_t& ball,
        const pqvec3_w_t<W>& a,
        const pqvec3_w_t<W>& b,
        const pqvec3_w_t<W>& c,
        const vec3_w_t<W>& n) noexcept {
    const auto bc = pqvec3_w_t<W>{ ball.centre };
    const auto bc_a = a - bc;
    const auto dot = m::dot(n, bc_a);

    // point on triangle plane closest to ball origin
    const auto p = m::fma(pqvec3_w_t<W>{ dot },n,bc);
    // is that point in triangle?
    const auto p_in_tri  = util::is_point_in_triangle(p, a, b, c);
    const auto p_in_ball = ball.contains(p);

    // edges in ball?
    const auto a_in_ball = ball.contains(a);
    const auto b_in_ball = ball.contains(b);
    const auto c_in_ball = ball.contains(c);

    // an edge is in ball, or triangle intersects the ball (closest point `p` is in ball and contained in triangle)
    // otherwise triangle does not intersect ball
    return a_in_ball || b_in_ball || c_in_ball || (p_in_tri && p_in_ball);
}

/**
 * @brief Ball-AABB intersection test. Checks for intersection and (strict) containment.
 *        The tests treat the AABB's shell differently:
 *        * intersection testing: the AABB is assumed to include its shell
 *        * containment testing: this test is strict, and the AABB is assumed to NOT include its shell
 */
inline intersect_ball_aabb_ret_t test_ball_aabb(
        const ball_t& ball,
        const aabb_t& aabb) noexcept {
    const auto r2 = m::sqr(ball.radius);
    const auto a = ball.centre - aabb.min;
    const auto b = aabb.max - ball.centre;

    const auto v = m::max(a, b);
    const auto v2 = m::dot(v,v);
    const auto contains = v2 < r2;

    const auto closest = m::clamp(ball.centre, aabb.min, aabb.max);
    const auto d = ball.centre - closest;
    const auto d2 = m::dot(d,d);
    const auto overlaps = d2 <= r2;

    return {
        .intersects = overlaps,
        .contains = contains,
    };
}

/**
 * @brief Ball-AABB intersection test. Checks for intersection and (strict) containment. Wide test.
 *        The tests treat the AABB's shell differently:
 *        * intersection testing: the AABB is assumed to include its shell
 *        * containment testing: this test is strict, and the AABB is assumed to NOT include its shell
 */
template <std::size_t W>
inline intersect_ball_aabb_w_ret_t<W> test_ball_aabb(
        const ball_t& ball,
        const pqvec3_w_t<W>& aabb_min,
        const pqvec3_w_t<W>& aabb_max) noexcept {
    const auto r2 = q_w_t<W,area_t>{ m::sqr(ball.radius) };
    const auto c  = pqvec3_w_t<W>{ ball.centre };

    const auto a = c - aabb_min;
    const auto b = aabb_max - c;

    const auto v = m::max(a, b);
    const auto v2 = m::dot(v,v);
    const auto contains = v2 < r2;

    const auto closest = m::clamp(c, aabb_min, aabb_max);
    const auto d = c - closest;
    const auto d2 = m::dot(d,d);
    const auto overlaps = d2 <= r2;

    return {
        .intersects_mask = overlaps,
        .contains_mask = contains,
    };
}

}
