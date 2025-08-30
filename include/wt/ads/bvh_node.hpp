/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 * Authors:  Umut Emre, Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#pragma once

#include <wt/math/shapes/aabb.hpp>
#include <wt/math/common.hpp>

#include "wt/ads/common.hpp"

namespace wt::ads::bvh {

struct node_t {
    aabb_t aabb = aabb_t::null();

    idx_t children_cluster_offset;
    bool is_leaf;

    idx_t tris_offset;
    idx_t tri_count;
};

struct node_cluster_t {
    std::array<node_t, 2> nodes;
};

}  // namespace wt::ads::bvh