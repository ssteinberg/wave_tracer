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

namespace wt {

/**
 * @brief 2D footprint of a beam-surface intersection.
 */
struct intersection_footprint_t {
    // direction of major axis
    dir2_t x = { 1,0 };
    // length of major and minor axes (a>=b)
    length_t la = 0*u::m, lb = 0*u::m;

    [[nodiscard]] inline pqvec2_t a() const noexcept {
        return x * la;
    }
    [[nodiscard]] inline pqvec2_t b() const noexcept {
        return y() * lb;
    }

    [[nodiscard]] inline dir2_t y() const noexcept {
        return dir2_t{ -x.y, x.x };
    }
};


/**
* @brief pdvs of UV w.r.t. beam footprint major and minor axes ``a`` and ``b``.
*/
struct intersection_uv_pdvs_t {
    f_t duda, dudb;
    f_t dvda, dvdb;
};

}
