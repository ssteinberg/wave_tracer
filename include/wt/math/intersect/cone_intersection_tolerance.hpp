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

#include <wt/math/shapes/aabb.hpp>
#include <wt/math/shapes/elliptic_cone.hpp>

namespace wt::intersect {

/**
 * @brief *Rough* estimate for the floating-point error in cone intersection tests.
 * @param primitive_aabb the AABB of the intersected primitive.
 */
constexpr inline auto cone_intersection_tolerance(const pqvec3_t& origin, const aabb_t& primitive_aabb) noexcept {
    // idea similar to `compute_intersection_triangle_fp_errors()` in intersection
    // constants are some ad hoc stuff
    // TODO: do better
    const f_t c0 = f_t(4e-7);
    const f_t c1 = f_t(1e-6);
    const f_t c2 = f_t(1e-6);

    // max world extent of primitive AABB
    const auto obj_extent = 2 * m::max(m::max_element(m::abs(primitive_aabb.min)), m::max_element(m::abs(primitive_aabb.max))); 

    // bound object-space error due to reconstruction and intersection 
    // and world-to-object transform 
    const auto obj_err = (c0+c2)*m::abs(origin) + pqvec3_t{ c1*obj_extent };
    // bound world-space error due to object-to-world transform 
    const auto wrd_err = (c1+c2)*m::abs(origin);

    return m::max_element(obj_err + wrd_err);
}

}
