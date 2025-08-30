/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#pragma once

#include <wt/math/shapes/aabb.hpp>
#include <wt/math/common.hpp>
#include <wt/math/shapes/elliptic_cone.hpp>
#include <wt/wt_context.hpp>

#include <wt/ads/ads.hpp>
#include "bvh8w_node.hpp"

namespace wt::ads {

namespace construction {
class bvh8w_constructor_t;
}

class bvh8w_t final : public ads_t {
    friend class construction::bvh8w_constructor_t;

public:
    using node_t = bvh8w::node_t;
    using leaf_node_t = bvh8w::leaf_node_t;

    struct tris_vectorized_data_t {
        std::vector<length_t> ax, ay, az;
        std::vector<length_t> bx, by, bz;
        std::vector<length_t> cx, cy, cz;
        std::vector<f_t> nx, ny, nz;
    };

private:
    const std::vector<node_t> nodes;
    const std::vector<leaf_node_t> leaf_nodes;
    const tris_vectorized_data_t vectorized_data;

    const aabb_t world;

    // some stats
    const f_t sah_cost, occupancy;
    const std::size_t max_depth;

public:
    bvh8w_t(std::vector<node_t> nodes,
            std::vector<leaf_node_t> leaf_nodes,
            tris_vectorized_data_t vectorized_data,
            std::vector<tri_t> tris,
            const aabb_t& world,
            f_t sah_cost,
            f_t occupancy,
            std::size_t max_depth) noexcept
        : ads_t(std::move(tris)),
          nodes(std::move(nodes)),
          leaf_nodes(std::move(leaf_nodes)),
          vectorized_data(std::move(vectorized_data)),
          world(world),
          sah_cost(sah_cost),
          occupancy(occupancy),
          max_depth(max_depth)
    {}

    [[nodiscard]] std::size_t nodes_count() const noexcept override { return nodes.size(); }

    [[nodiscard]] inline const auto& vectorized_tri_data() const noexcept {
        return vectorized_data;
    }

    [[nodiscard]] inline const std::int32_t root_ptr() const noexcept {
        return 1;
    }
    [[nodiscard]] inline const node_t& node(idx_t nidx) const noexcept {
        return nodes[nidx];
    }
    [[nodiscard]] inline const leaf_node_t& leaf_node(idx_t nidx) const noexcept {
        return leaf_nodes[nidx];
    }

    [[nodiscard]] const aabb_t& V() const noexcept override {
        return world;
    }

    /**
     * @brief Intersects the ADS with ball, returning the intersection record and contained primitives.
     */
    [[nodiscard]] intersection_record_t intersect(
        const ball_t &ball,
        const intersect_opts_t& opts = intersect_opts_t::defaults()) const noexcept override;

    /**
     * @brief Intersects the ADS with a ray, returning the intersection record
     * with the intersected  primitive.
     *
     * @param range traversal bounds
     * @param z_search_range_scale scaler for z-axis search range
     */
    [[nodiscard]] intersection_record_t intersect(
        const ray_t &ray,
        const pqrange_t<> range = { 0 * u::m, limits<length_t>::infinity() }) const noexcept override;

    /**
     * @brief Intersects the ADS with a cone, returning the intersection record
     * and contained primitives. Once the closest intersection is found, looks
     * for triangles within a z distance from the closest point. This distance
     * is computed as the cone major axis length time 'z_search_range_scale'.
     *
     * @param range traversal bounds
     * @param z_search_range_scale scaler for z-axis search range
     */
    [[nodiscard]] intersection_record_t intersect(
        const elliptic_cone_t &cone,
        const pqrange_t<> range = { 0 * u::m, limits<length_t>::infinity() },
        const intersect_opts_t& opts = intersect_opts_t::defaults()) const noexcept override;

    /**
     * @brief Intersects the ADS with a ray. Returns TRUE if a hit was found.
     *
     * @param traversal_mode traversal mode (front/back/font-and-back faces)
     * @param range traversal bounds
     */
    [[nodiscard]] bool shadow(
        const ray_t &ray, const pqrange_t<> range) const noexcept override;

    /**
     * @brief Intersects the ADS with a cone. Returns TRUE if a hit was found.
     *
     * @param range traversal bounds
     */
    [[nodiscard]] bool shadow(
        const elliptic_cone_t &cone, const pqrange_t<> range) const noexcept override;

    [[nodiscard]] scene::element::info_t description() const override;
};

}
