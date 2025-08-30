/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#pragma once

#include <cstdint>
#include <wt/ads/common.hpp>
#include <wt/math/simd/wide_vector.hpp>

namespace wt::ads::bvh8w {

struct leaf_node_t {
    idx_t tris_ptr, count;
};

static constexpr std::size_t aabbs_per_node = 8;

/**
 * @brief 8 AABBs and 8 31-bit pointers + 1-bit leaf flag.
 */
struct node_t {
    pqvec3_w_t<aabbs_per_node> min,max;

    /** @brief Child pointers:
     *         * 0 - empty
     *         * >0 - child node ptr
     *         * <0 - leaf node ptr
     */
    std::int32_t child_ptrs[aabbs_per_node] = { 0,0,0,0,0,0,0,0 };   // zero indicates empty

    std::uint32_t tris_start, tris_count=0;

    // 24byte padding at the end
    // TODO: can we do anything with this space?
};

}
