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
#include <set>
#include <cmath>

#include <wt/math/common.hpp>

#include <wt/math/intersect/intersect_defs.hpp>
#include <wt/math/range.hpp>

#include "common.hpp"

namespace wt::ads {

/**
 * @brief Contains the data of an ADS intersection query, including the lists of intersected triangles and edges.
 */
struct intersection_record_t {
public:
    using triangles_container_t = std::vector<tuid_t>;
    using edges_container_t = std::set<tuid_t>;

    struct rt_record_t {
        intersect::intersect_ray_tri_ret_t raytracing_intersection_record;
        tuid_t tuid;
    };

    struct triangles_accessor_t {
        const tuid_t* s = nullptr;
        const tuid_t* e = nullptr;
        [[nodiscard]] const auto begin() const noexcept { return s; }
        [[nodiscard]] const auto end() const noexcept   { return e; }
        [[nodiscard]] const auto size() const noexcept  { return std::distance(s,e); }
        [[nodiscard]] const auto empty() const noexcept { return size()==0; }
    };

private:
    /** @brief intersection dist */
    length_t dist;

    struct containers_t {
        const triangles_container_t* tris_container;
        const edges_container_t* edges_container;
    };
    union {
        rt_record_t rt_record{};
        containers_t data;
    };
    
    bool front_face;
    bool has_rt_record = false;

public:
    // intersection with multiple triangles or edges
    intersection_record_t(length_t dist, bool front_face,
                          const triangles_container_t* triangles,
                          const edges_container_t* edges) noexcept
        : dist(dist),
          data{
            triangles, edges
          },
          front_face(front_face)
    {
        assert(!!data.tris_container);
    }
    // ray-tracing intersection with single triangle
    intersection_record_t(const intersect::intersect_ray_tri_ret_t& raytracing_intersection_record,
                          bool front_face,
                          const tuid_t tuid,
                          const edges_container_t* edges) noexcept
        : dist(raytracing_intersection_record.dist),
          rt_record{ raytracing_intersection_record, tuid },
          front_face(front_face),
          has_rt_record(true)
    {}
    // no intersection
    intersection_record_t() noexcept
        : dist{ -limits<length_t>::infinity() },
          data{ nullptr,nullptr }
    {}

    intersection_record_t(const intersection_record_t& o) noexcept = default;

    /**
     * @brief Distance to first intersection, or INF if none.
     */
    [[nodiscard]] inline auto distance() const noexcept { return dist; }

    /**
     * @brief Returns TRUE if first intersection is front facing.
     */
    [[nodiscard]] inline bool is_front_face() const noexcept { return front_face; }

    /**
     * @brief Returns TRUE if this intersection record holds no triangles or edges.
     */
    [[nodiscard]] inline bool empty() const noexcept {
        return !has_rt_record && (!data.tris_container || data.tris_container->empty());
    }

    [[nodiscard]] inline bool has_raytracing_intersection_record() const noexcept {
        return has_rt_record;
    }

    [[nodiscard]] inline const auto& get_raytracing_intersection_record() const noexcept {
        assert(has_raytracing_intersection_record());
        return rt_record.raytracing_intersection_record;
    }

    /**
     * @brief Returns accessor for intersected triangles.
     *        The underlying storage might use thread_local vectors; a user should finish accessing the intersection record before calling adds::intersect on the same ads again.
     */
    [[nodiscard]] inline auto triangles() const noexcept {
        if (has_rt_record)
            return triangles_accessor_t{ .s=&rt_record.tuid, .e=&rt_record.tuid+1 };
        else if (data.tris_container && !data.tris_container->empty())
            return triangles_accessor_t{ .s=&data.tris_container->front(), .e=&data.tris_container->front() + data.tris_container->size() };
        return triangles_accessor_t{};
    }

    /**
     * @brief Returns container of intersected edges.
     *        The underlying storage might use thread_local vectors; a user should finish accessing the intersection record before calling adds::intersect on the same ads again.
     */
    [[nodiscard]] inline const auto& edges() const noexcept {
        static const edges_container_t dummy;
        return !has_rt_record && data.edges_container ? *data.edges_container : dummy;
    }
};

}
