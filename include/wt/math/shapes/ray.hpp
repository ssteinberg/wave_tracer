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
#include <wt/math/frame.hpp>
#include <wt/math/shapes/aabb.hpp>

#include <wt/util/assert.hpp>

namespace wt {

struct ray_t {
    pqvec3_t o;       // origin
    dir3_t d;        // direction

    // precomputed auxiliary values
    vec3_t invd;

    constexpr ray_t(const pqvec3_t& origin,
                    const dir3_t& direction)
        : o(origin),
          d(direction),
          invd(f_t(1)/direction)
    {}
    constexpr ray_t(const ray_t&) = default;
    ray_t& operator=(const ray_t&) = default;

    [[nodiscard]] inline auto propagate(const length_t dist) const noexcept {
        return o + dist*d;
    }
};

}
