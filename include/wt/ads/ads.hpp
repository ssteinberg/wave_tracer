/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once


#include <wt/wt_context.hpp>
#include <wt/scene/element/info.hpp>

#include <wt/math/shapes/ball.hpp>
#include <wt/math/range.hpp>
#include <wt/math/shapes/elliptic_cone.hpp>

#include "common.hpp"
#include "intersection_record.hpp"

namespace wt::ads {

/**
 * @brief Generic accelerating data structure (ADS) interface.
 */
class ads_t {
public:
    struct intersect_opts_t {
        bool detect_edges;
        bool accumulate_edges;
        bool accumulate_triangles;
        f_t z_search_range_scale;

        static constexpr inline intersect_opts_t defaults() noexcept {
            return {
                .detect_edges = true,
                .accumulate_edges = false,
                .accumulate_triangles = false,
                .z_search_range_scale = 1,
            };
        }
    };

protected:
    std::vector<edge_t> edges;
    std::vector<tri_t> tris;

public:
    ads_t() noexcept = default;
    ads_t(std::vector<tri_t> tris) noexcept : tris(std::move(tris)) {}
    ads_t(ads_t&&) = default;
    virtual ~ads_t() noexcept = default;
    
    [[nodiscard]] inline const tri_t& tri(tuid_t tuid) const noexcept {
        return tris[tuid];
    }
    [[nodiscard]] inline const edge_t& edge(std::uint32_t euid) const noexcept {
        return edges[euid];
    }

    [[nodiscard]] inline auto triangles_count() const noexcept { return tris.size(); }
    [[nodiscard]] virtual std::size_t nodes_count() const noexcept = 0;

    [[nodiscard]] virtual const aabb_t& V() const noexcept = 0;

    /**
     * @brief Intersects the ADS with ball, returning the intersection record and contained primitives.
     */
    [[nodiscard]] virtual intersection_record_t intersect(
            const ball_t &ball,
            const intersect_opts_t& opts = intersect_opts_t::defaults()) const noexcept = 0;

    /**
     * @brief Intersects the ADS with a ray, returning the intersection record with the intersected  primitive.
     *
     * @param range traversal bounds
     * @param z_search_range_scale scaler for z-axis search range
     */
    [[nodiscard]] virtual intersection_record_t intersect(
            const ray_t &ray,
            const pqrange_t<> range = { 0*u::m, limits<length_t>::infinity() }) const noexcept = 0;

    /**
     * @brief Intersects the ADS with an elliptic cone, returning the intersection record and contained primitives.
     *        Once the closest intersection is found, looks for triangles within a z distance from the closest point. This distance is computed as the cone major axis length time 'z_search_range_scale'.
     *
     * @param range traversal bounds
     * @param z_search_range_scale scaler for z-axis search range
     */
    [[nodiscard]] virtual intersection_record_t intersect(
            const elliptic_cone_t &cone,
            const pqrange_t<> range = { 0*u::m, limits<length_t>::infinity() },
            const intersect_opts_t& opts = intersect_opts_t::defaults()) const noexcept = 0;

    /**
     * @brief Intersects the ADS with a ray. Returns TRUE if a hit was found.
     *
     * @param range traversal bounds
     */
    [[nodiscard]] virtual bool shadow(
            const ray_t &ray,
            const pqrange_t<> range) const noexcept = 0;

    /**
     * @brief Intersects the ADS with an elliptical cone. Returns TRUE if a hit was found.
     *
     * @param range traversal bounds
     */
    [[nodiscard]] virtual bool shadow(
            const elliptic_cone_t &cone,
            const pqrange_t<> range) const noexcept = 0;

    [[nodiscard]] virtual scene::element::info_t description() const = 0;
};

}
