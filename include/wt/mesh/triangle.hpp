/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <array>
#include <optional>

#include <wt/math/common.hpp>
#include <wt/math/encoded_normal.hpp>
#include <wt/mesh/surface_differentials.hpp>

namespace wt::mesh {

struct alignas(64) triangle_t {
    std::array<pqvec3_t,3> p;

    dir3_t geo_n;
    std::array<encoded_normal_t,3> n;

    std::optional<std::array<vec2_t,3>> uv;

    surface_differentials_t tangent_frame;
};

}
