/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#pragma once

#include <wt/math/simd/wide_vector.hpp>

#include <wt/math/shapes/aabb.hpp>
#include "bvh8w_node.hpp"

namespace wt::ads::bvh8w {

struct bvh8w_aabbs_t {
    pqvec3_w_t<aabbs_per_node> min,max;
};

[[nodiscard]] inline auto node_aabbs(const node_t& node) noexcept {
    return bvh8w_aabbs_t{
        node.min, node.max
    };
}

[[nodiscard]] inline bool is_ptr_empty(std::int32_t ptr) noexcept {
    return ptr==0;
}
[[nodiscard]] inline bool is_ptr_leaf(std::int32_t ptr) noexcept {
    return ptr<0;
}
[[nodiscard]] inline bool is_ptr_child(std::int32_t ptr) noexcept {
    return ptr>0;
}
[[nodiscard]] inline idx_t leaf_node_ptr(std::int32_t ptr) noexcept {
    return (-ptr)-1;
}
[[nodiscard]] inline idx_t child_node_ptr(std::int32_t ptr) noexcept {
    return ptr-1;
}

}

