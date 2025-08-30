/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <vector>

#include <wt/math/common.hpp>
#include <wt/math/shapes/elliptic_cone.hpp>

#include "ads.hpp"
#include "intersection_record.hpp"
#include "common.hpp"

namespace wt::ads {

struct intersection_work_tri_t {
    tuid_t tuid;
    length_t dist;

    inline bool operator<(const intersection_work_tri_t& o) const noexcept {
        return tuid<o.tuid;
    }
    inline explicit operator tuid_t() const noexcept {
        return tuid;
    }
};
struct intersection_ray_work_tri_t {
    tuid_t tuid;
    length_t dist;
    bool front_face;

    inline bool operator<(const intersection_work_tri_t& o) const noexcept {
        return tuid<o.tuid;
    }
    inline explicit operator tuid_t() const noexcept {
        assert(dist>=zero);
        return tuid;
    }
};

struct intersection_record_ray_work_t {
    intersection_ray_work_tri_t triangle;
    intersect::intersect_ray_tri_ret_t intersection = {};
    const pqrange_t<> range = { 0*u::m, limits<length_t>::infinity() };

    intersection_record_ray_work_t(const pqrange_t<> range={}) noexcept
        : range(range)
    {
        triangle.dist = limits<length_t>::infinity();
    }
};

template <typename TriangleContainer>
struct intersection_record_work_t {
    static thread_local TriangleContainer triangles;
    
    length_t intr_dist = limits<length_t>::infinity();
    f_t z_search_range_scale;
    bool front_face = false;

    const pqrange_t<> searchrange = { 0*u::m, limits<length_t>::infinity() };

    intersection_record_work_t(const pqrange_t<> range={},
                               const f_t z_search_range_scale=1) noexcept
        : z_search_range_scale(z_search_range_scale),
          searchrange(range)
    {
        triangles.clear();
    }

    [[nodiscard]] inline auto search_range(const elliptic_cone_t& cone) const noexcept {
        const auto dist = m::max(searchrange.min, intr_dist);
        const auto z_dist = cone.axes(dist).x * z_search_range_scale;
        const auto range = pqrange_t<>{ searchrange.min, m::min(searchrange.max, dist+z_dist) } & pqrange_t<>::positive();
        assert(range.empty() || range.max>=range.min);
        return range;
    }
};

template <typename TriangleContainer>
thread_local TriangleContainer intersection_record_work_t<TriangleContainer>::triangles;


/**
 * @brief Helper to convert intersection_record_ray_work_t to intersection_record_t
 */
template <std::derived_from<ads_t> Ads>
[[nodiscard]] inline intersection_record_t ray_work_to_intersection_record(
        const Ads& ads,
        const intersection_record_ray_work_t& work,
        const pqrange_t<> traversal_range) noexcept {
    // remove too far-away triangles
    if (!m::isfinite(work.triangle.dist) || work.triangle.dist>traversal_range.max) return {};
    
    assert(traversal_range.contains(work.triangle.dist));

    const auto tuid = static_cast<tuid_t>(work.triangle);
    return {
        work.intersection,
        work.triangle.front_face,
        tuid, nullptr
    };
}

/**
 * @brief Helper to convert intersection_record_work_t to intersection_record_t for an 
 * a beam intersection test.
 */
template <typename TriangleContainer, std::derived_from<ads_t> Ads>
[[nodiscard]] inline intersection_record_t cone_work_to_intersection_record(
        const Ads& ads,
        intersection_record_work_t<TriangleContainer> work,
        const elliptic_cone_t &cone,
        const ads_t::intersect_opts_t& opts) noexcept {
    const auto range = work.search_range(cone);
    
    static thread_local std::vector<tuid_t> triangles;
    static thread_local std::set<tuid_t> edges;

    // clear static structures, if we are not accumulating
    if (!opts.accumulate_triangles)
        triangles.clear();
    if (!opts.accumulate_edges)
        edges.clear();

    for (const auto& wt : work.triangles) {
        // remove too far-away triangles
        if (wt.dist>range.max) continue;

        triangles.push_back(static_cast<tuid_t>(wt));
        
        // find edges
        if (opts.detect_edges) {
            const auto& tri = ads.tri(wt.tuid);
            if (tri.edge_ab) edges.insert(tri.edge_ab);
            if (tri.edge_bc) edges.insert(tri.edge_bc);
            if (tri.edge_ca) edges.insert(tri.edge_ca);
        }
    }

    return { work.intr_dist, work.front_face, &triangles, &edges };
}

/**
 * @brief Helper to convert intersection_record_work_t to intersection_record_t for a ball intersection test
 */
template <typename TriangleContainer, std::derived_from<ads_t> Ads>
[[nodiscard]] inline intersection_record_t ball_work_to_intersection_record(
        const Ads& ads,
        intersection_record_work_t<TriangleContainer> work,
        const ads_t::intersect_opts_t& opts) noexcept {
    static thread_local std::vector<tuid_t> triangles;
    static thread_local std::set<tuid_t> edges;

    // clear static structures, if we are not accumulating
    if (!opts.accumulate_triangles)
        triangles.clear();
    if (!opts.accumulate_edges)
        edges.clear();

    for (const auto& wt : work.triangles) {
        triangles.push_back(static_cast<tuid_t>(wt));

        // find edges
        if (opts.detect_edges) {
            const auto& tri = ads.tri(wt.tuid);
            if (tri.edge_ab) edges.insert(tri.edge_ab);
            if (tri.edge_bc) edges.insert(tri.edge_bc);
            if (tri.edge_ca) edges.insert(tri.edge_ca);
        }
    }

    return { 0*u::m, false, &triangles, &edges };
}

}
