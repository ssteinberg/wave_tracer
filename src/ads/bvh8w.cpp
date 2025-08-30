/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#include <bitset>

#include <wt/ads/bvh8w/bvh8w.hpp>
#include <wt/ads/bvh8w/common.hpp>

#include <wt/ads/ads_stats.hpp>
#include <wt/ads/traversal_common.hpp>

#include <wt/math/intersect/intersect_defs.hpp>
#include <wt/math/intersect/ball.hpp>
#include <wt/math/simd/wide_vector.hpp>

#include <wt/scene/element/attributes.hpp>


using namespace wt;
using namespace wt::ads;


static constexpr auto ray_traversal_treat_node_as_leaf_if_triangle_count_lt = 16;


using intersection_record_vec_work_t = intersection_record_work_t<std::vector<intersection_work_tri_t>>;

struct stack_node_ptr_t {
    length_t min_range;
    int32_t ptr;
};

struct cluster_intersect_t {
    length_w8_t tmins;
    std::bitset<8> result_mask;
};

// simple insertion sort
inline void stack_sorter(auto* stack, int size) noexcept {
    constexpr auto cmp = [](const auto& n1, const auto& n2) {
        return n1.min_range > n2.min_range;
    };
    for (int i=1; i<size; ++i) {
        const auto p = stack[i];
        int j;
        for (j=i-1; j>=0 && cmp(p,stack[j]); --j)
            stack[j+1] = stack[j];

        stack[j+1] = p;
    }
}


/**
 * General loading routines
 */

struct tri_cluster_8w_t {
    pqvec3_w8_t a;
    pqvec3_w8_t b;
    pqvec3_w8_t c;
    vec3_w8_t   n;
};

inline auto load_tri_cluster_8w(const bvh8w_t* ads,
                                const idx_t tidx) noexcept {
    tri_cluster_8w_t data;
    data.a = pqvec3_w8_t{
        ads->vectorized_tri_data().ax.data()+tidx, 
        ads->vectorized_tri_data().ay.data()+tidx,
        ads->vectorized_tri_data().az.data()+tidx,
        simd::unaligned_data
    };
    data.b = pqvec3_w8_t{
        ads->vectorized_tri_data().bx.data()+tidx, 
        ads->vectorized_tri_data().by.data()+tidx,
        ads->vectorized_tri_data().bz.data()+tidx,
        simd::unaligned_data
    };
    data.c = pqvec3_w8_t{
        ads->vectorized_tri_data().cx.data()+tidx, 
        ads->vectorized_tri_data().cy.data()+tidx,
        ads->vectorized_tri_data().cz.data()+tidx,
        simd::unaligned_data
    };
    data.n = vec3_w8_t{
        ads->vectorized_tri_data().nx.data()+tidx, 
        ads->vectorized_tri_data().ny.data()+tidx,
        ads->vectorized_tri_data().nz.data()+tidx,
        simd::unaligned_data
    };

    return data;
}


/**
 * Cone traversal routines
 */

struct cone_cluster_intersect_data_t {
    pqvec3_w8_t ro;
    vec3_w8_t   rd;
    vec3_w8_t   rinvd;
    f_w8_t      ta;
    length_w8_t ix;

    cone_cluster_intersect_data_t(const elliptic_cone_t& cone) noexcept
        : ro(cone.o()),
          rd(vec3_t{ cone.d() }),
          rinvd(cone.ray().invd),
          ta(cone.get_tan_alpha()),
          ix(cone.x0())
    {}
};

template <bool shadow>
inline bool gather_tris(const bvh8w_t* tree,
                        const elliptic_cone_t &cone,
                        const cone_cluster_intersect_data_t& m256data,
                        const pqrange_t<>& range,
                        const std::uint32_t t0,
                        const std::uint32_t tcount,
                        const ads_t::intersect_opts_t& opts,
                        intersection_record_vec_work_t &record) noexcept {
    bool found_intersection = false;

    // do tests against individual tris.
    // this is by far the slowest part of cone traversal
    // TODO: vectorize the following
    for (std::size_t t=0; t<tcount; ++t) {
        const auto tuid = (tuid_t)(t0+t);
        const auto& tri = tree->tri(tuid);

        bool front_face;
        if constexpr (!shadow) {
            front_face = m::dot(tri.n,-cone.ray().d)>zero;
        }

        // intersect

        if constexpr (shadow) {
            if (ads_stats::test_cone_tri(cone, tri.a, tri.b, tri.c, range)) {
                record.intr_dist = range.min;
                return true;
            }
            continue;
        }

        assert(range.min>=zero);

        const auto intrs = ads_stats::intersect_cone_tri(
                cone,
                tri.a, tri.b, tri.c, tri.n,
                range);
        if (intrs) {
            const auto& dist = intrs->dist;
            assert(range.max>=dist);
            if (dist>range.max)  // can happen due to numerics
                continue;

            // keep track of closest intersection
            if (dist<record.intr_dist) {
                record.intr_dist = dist;
                record.front_face = front_face;
            }
            found_intersection = true;

            record.triangles.emplace_back(intersection_work_tri_t{
                .tuid = tuid,
            });
        }
    }

    if constexpr (shadow)
        return false;
    else
        return found_intersection;
}

inline cluster_intersect_t cone_cluster_intersect(
        const bvh8w_t* tree,
        const pqrange_t<>& range,
        const cone_cluster_intersect_data_t& data,
        const bvh8w::bvh8w_aabbs_t& aabbs8w) noexcept {
    auto aabb_o_min = aabbs8w.min - data.ro;
    auto aabb_o_max = aabbs8w.max - data.ro;

    const auto b = m::selectv(aabb_o_max, aabb_o_min, bvec3_w8_t{ data.rinvd });

    // compute dot(d,b) -- farthest z distance from cone origin
    const auto dot_d_b = m::dot(data.rd, b);
    const auto maxz = m::clamp(dot_d_b, length_w8_t::zero(), length_w8_t{ range.max });

    // cone cross section at maxz
    const auto enlr = m::fma(maxz, data.ta, data.ix);

    // ray-aabb intersections against enlarged AABBs
    aabb_o_min = aabb_o_min - (pqvec3_w8_t)enlr;
    aabb_o_max = aabb_o_max + (pqvec3_w8_t)enlr;

    const auto ae = m::selectv(aabb_o_min, aabb_o_max, bvec3_w8_t{ data.rinvd });
    const auto be = m::selectv(aabb_o_max, aabb_o_min, bvec3_w8_t{ data.rinvd });
    const auto dmin = ae * data.rinvd;
    const auto dmax = be * data.rinvd;

    auto tmin = length_w8_t::zero();
    auto tmax = dmax.x();
    tmin = m::max(tmin, dmin.x());
    tmax = m::min(tmax, dmax.y());
    tmin = m::max(tmin, dmin.y());
    tmax = m::min(tmax, dmax.z());
    tmin = m::max(tmin, dmin.z());

    const auto cond1 = tmin <= tmax;
    const auto cond2 = tmax >= length_w8_t{ range.min };
    const auto cond3 = tmin <= length_w8_t{ range.max };
    const auto result = cond1 && cond2 && cond3;

    return {
        .tmins = tmin,
        .result_mask = result.to_bitmask()
    };
}

template <bool shadow>
inline bool traverse(const bvh8w_t* tree,
                     const elliptic_cone_t& cone,
                     const ads_t::intersect_opts_t& opts,
                     intersection_record_vec_work_t &record,
                     int& internal_nodes, int& leaf_nodes, int& subtrees) noexcept {
    const auto cluster_intersect_data = cone_cluster_intersect_data_t{ cone };
    auto range = record.search_range(cone);

    constexpr auto stack_size = 128;
    stack_node_ptr_t stack[stack_size];
    int s=1;
    stack[0] = {
        .min_range = 0 * u::m,
        .ptr = tree->root_ptr(),
    };

    // unwinds irrelevant nodes
    const auto unwind_stack = [&]() {
        range = record.search_range(cone);
        while (s>0 &&
               stack[s-1].min_range >= range.max)
            --s;
    };

    for (;s>0;) {
        if (bvh8w::is_ptr_leaf(stack[s-1].ptr)) {
            if constexpr (ads_stats::additional_ads_counters)
                ++leaf_nodes;

            const auto& leaf = tree->leaf_node(bvh8w::leaf_node_ptr(stack[s-1].ptr));
            // find closest intersecting tris in leaf (if exists)
            const bool intr = gather_tris<shadow>(tree, cone, cluster_intersect_data, range,
                                                  leaf.tris_ptr, leaf.count,
                                                  opts, record);
            --s;

            if (intr) {
                // return immediately for shadow queries
                if constexpr (shadow)
                    return true;
                unwind_stack();
            }
        }
        else {
            if constexpr (ads_stats::additional_ads_counters)
                ++internal_nodes;

            // traverse node
            const auto& n = tree->node(bvh8w::child_node_ptr(stack[s-1].ptr));
            const auto& aabbs8w = bvh8w::node_aabbs(n);
            --s;

            // TODO: subtree traversal for nodes contained in cone
            // This is important for edge cases of detailed geometry and cones with growing apertures,
            // however it is difficult to do so in a way that is a performance win: testing for containment is expensive
    
            const auto r = cone_cluster_intersect(tree, range,
                                                  cluster_intersect_data, aabbs8w);
            // collect stats
            ads_stats::on_ray_aabb_8w_test();

            // gather intersected children
            int begin = s;
            for (int i=0;i<8;++i) {
                const auto& ptr = n.child_ptrs[i];
                if (r.result_mask[i]==0 || bvh8w::is_ptr_empty(ptr))
                    continue;

                auto t = r.tmins.read(i);
                if (t >= range.max)
                    continue;

#ifndef RELEASE
                if (s==stack_size) std::exit(99); // stack overflow
#endif
                stack[s++] = { t, ptr };
            }
            
            // sort nodes in descending order
            [[assume(s-begin<=8)]];
            stack_sorter(&stack[begin], s-begin);
        }
    }

    return !record.triangles.empty();
}

intersection_record_t bvh8w_t::intersect(const elliptic_cone_t &cone,
                                         const pqrange_t<> traversal_range,
                                         const intersect_opts_t& opts) const noexcept {
    assert(traversal_range.max>0*u::m);

    std::chrono::high_resolution_clock::time_point start;
    if constexpr (ads_stats::additional_ads_counters)
        start = std::chrono::high_resolution_clock::now();

    auto work = intersection_record_vec_work_t{ traversal_range,opts.z_search_range_scale };

    // traverse BVH
    static constexpr bool shadow = false;
    int internal_nodes = 0, leaf_nodes = 0, subtrees = 0;
    ::traverse<shadow>(this, cone, opts, work, internal_nodes,leaf_nodes,subtrees);

    auto ret = cone_work_to_intersection_record(*this, work, cone, opts);

    // record cone cast
    ads_stats::on_cone_cast_event(
            !ret.empty(), 
            ret.empty() && !V().contains(cone.ray().propagate(traversal_range.max)),
            ret.triangles().size(),
            start,
            internal_nodes,leaf_nodes,subtrees);

    return ret;
}

bool bvh8w_t::shadow(const elliptic_cone_t &cone, const pqrange_t<> traversal_range) const noexcept {
    if (traversal_range.empty())
        return false;

    std::chrono::high_resolution_clock::time_point start;
    if constexpr (ads_stats::additional_ads_counters)
        start = std::chrono::high_resolution_clock::now();

    auto work = intersection_record_vec_work_t{ traversal_range };

    // traverse BVH
    static constexpr bool shadow = true;
    int internal_nodes = 0, leaf_nodes = 0, subtrees = 0;
    ::traverse<shadow>(this, cone, { .detect_edges = false }, work, internal_nodes,leaf_nodes,subtrees);

    const bool found_intersection = m::isfinite(work.intr_dist);

    // record cone cast
    ads_stats::on_shadow_cone_cast_event(
            found_intersection,
            !found_intersection && !V().contains(cone.ray().propagate(traversal_range.max)),
            start,
            internal_nodes,leaf_nodes,subtrees);

    assert(found_intersection || traversal_range.contains(work.intr_dist));
    return !found_intersection;
}


/**
 * Ray traversal routines
 */

struct ray_cluster_intersect_data_t {
    pqvec3_w8_t ro;
    vec3_w8_t   rd;
    vec3_w8_t   rinvd;

    ray_cluster_intersect_data_t(const ray_t& ray) noexcept
        : ro(ray.o),
          rd(vec3_t{ ray.d }),
          rinvd(ray.invd)
    {}
};

template <bool shadow>
inline bool gather_tris(const bvh8w_t* ads,
                        const ray_cluster_intersect_data_t& rdata,
                        const idx_t t0ptr, const idx_t count,
                        const pqrange_t<>& range,
                        intersection_record_ray_work_t &record) noexcept {
    bool intersects = false;
    for (auto t=0ul; t<count; t+=8) {
        const auto tidx = t0ptr+t;
        const auto tris = load_tri_cluster_8w(ads, tidx);

        b_w8_t b_front_face_mask;
        if constexpr (!shadow)
            b_front_face_mask = m::dot(tris.n, rdata.rd) <= f_w8_t::zero();

        // intersect

        if constexpr (shadow) {
            const auto intrs8 = ads_stats::test_ray_tri_8w(rdata.ro,
                                                           rdata.rd,
                                                           tris.a,
                                                           tris.b,
                                                           tris.c,
                                                           range);
            if (m::any(intrs8)) {
                record.triangle.dist = range.min;
                return true;
            }
            continue;
        }

        const auto intrs8 =
            ads_stats::intersect_ray_tri_8w(rdata.ro,
                                            rdata.rd,
                                            tris.a,
                                            tris.b,
                                            tris.c,
                                            range);
        const auto b_front_face = b_front_face_mask.to_bitmask();
        for (auto i=0; i<m::min<int>(8,count-t); ++i) {
            const auto dist  = intrs8.result.read(i);
            const bool intrs = dist != -limits<length_t>::infinity();
            if (intrs && dist<record.triangle.dist) {
                record.intersection = intersect::intersect_ray_tri_ret_t{
                    .dist = dist,
                    .bary = barycentric_t(vec2_t{ intrs8.baryx.read(i),intrs8.baryy.read(i) })
                };
                record.triangle = {
                    .tuid = (tuid_t)(tidx+i),
                    .dist = dist,
                    .front_face = b_front_face[i],
                };
                intersects = true;
            }
        }
    }

    return intersects;
}

inline cluster_intersect_t ray_cluster_intersect(
        const bvh8w_t* tree,
        const length_t max_dist,
        const ray_cluster_intersect_data_t& data,
        const bvh8w::bvh8w_aabbs_t& aabbs8w) noexcept {
    const auto ira = intersect::intersect_ray_aabb_fast(
            data.ro, data.rinvd, aabbs8w.min, aabbs8w.max,
            pqrange_t<>{ 0*u::m, max_dist });

    return {
        .tmins = ira.min,
        .result_mask = ira.mask.to_bitmask(),
    };
}

template <bool shadow>
inline bool traverse(const bvh8w_t* tree,
                     const ray_t& ray,
                     intersection_record_ray_work_t &record,
                     int& nodes) noexcept {
    const auto cluster_intersect_data = ray_cluster_intersect_data_t{ ray };
    
    constexpr auto stack_size = 64;
    stack_node_ptr_t stack[stack_size];
    int s=1;

    const auto unwind_stack = [&]() {
        // unwind irrelevant nodes
        while (s>0 &&
               stack[s-1].min_range >= record.triangle.dist)
            --s;
    };

    stack[0] = {
        .min_range = 0 * u::m,
        .ptr = tree->root_ptr(),
    };
    for (;s>0;) {
        if (bvh8w::is_ptr_leaf(stack[s-1].ptr)) {
            const auto& leaf = tree->leaf_node(bvh8w::leaf_node_ptr(stack[s-1].ptr));
            // find closest intersecting tri in leaf (if exists)
            const bool intr = gather_tris<shadow>(tree, cluster_intersect_data,
                                                  leaf.tris_ptr, leaf.count,
                                                  record.range, record);
            --s;

            if (intr) {
                // return immediately for shadow queries
                if constexpr (shadow)
                    return true;
                unwind_stack();
            }
        }
        else {
            // traverse node
            const auto& n = tree->node(bvh8w::child_node_ptr(stack[s-1].ptr));
            --s;

            const bool should_traverse_as_leaf = n.tris_count <= ray_traversal_treat_node_as_leaf_if_triangle_count_lt;
            if (should_traverse_as_leaf) {
                // find closest intersecting tri in subtree (if exists)
                const bool intr = gather_tris<shadow>(tree, cluster_intersect_data,
                                                      n.tris_start, n.tris_count,
                                                      record.range, record);

                if (intr) {
                    // return immediately for shadow queries
                    if constexpr (shadow)
                        return true;
                    unwind_stack();
                }
                continue;
            }

            if constexpr (ads_stats::additional_ads_counters)
                ++nodes;

            const auto& aabbs8w = bvh8w::node_aabbs(n);
            const auto r = ray_cluster_intersect(tree, record.triangle.dist,
                                                 cluster_intersect_data, aabbs8w);
            // collect stats
            ads_stats::on_ray_aabb_8w_test();

            // gather intersected children
            int begin = s;
            for (int i=0;i<8;++i) {
                if (r.result_mask[i]!=0 && !bvh8w::is_ptr_empty(n.child_ptrs[i])) {
#ifndef RELEASE
                    if (s==stack_size) std::exit(99); // stack overflow
#endif
                    stack[s++] = { r.tmins.read(i), n.child_ptrs[i] };
                }
            }
            // sort nodes in descending order
            [[assume(s-begin<=8)]];
            stack_sorter(&stack[begin], s-begin);
        }
    }

    return record.triangle.dist < limits<length_t>::infinity();
}

intersection_record_t bvh8w_t::intersect(
    const ray_t &ray,
    const pqrange_t<> traversal_range) const noexcept {
    assert(traversal_range.max > zero);

    std::chrono::high_resolution_clock::time_point start;
    if constexpr (ads_stats::additional_ads_counters)
        start = std::chrono::high_resolution_clock::now();

    auto work = intersection_record_ray_work_t{traversal_range};

    // traverse bvh8w
    static constexpr bool shadow = false;
    int nodes = 0;
    ::traverse<shadow>(this, ray, work, nodes);

    // record ray cast
    ads_stats::on_ray_cast_event(m::isfinite(work.triangle.dist) && work.triangle.dist<=traversal_range.max, 
                                 !m::isfinite(work.triangle.dist) && !V().contains(ray.propagate(traversal_range.max)),
                                 false,
                                 start,
                                 nodes);

    return ray_work_to_intersection_record(*this, work, traversal_range);
}

bool bvh8w_t::shadow(const ray_t &ray, const pqrange_t<> traversal_range) const noexcept {
    auto work = intersection_record_ray_work_t{traversal_range};

    std::chrono::high_resolution_clock::time_point start;
    if constexpr (ads_stats::additional_ads_counters)
        start = std::chrono::high_resolution_clock::now();

    // traverse bvh8w
    static constexpr bool shadow = true;
    int nodes = 0;
    ::traverse<shadow>(this, ray, work, nodes);

    // record ray cast
    ads_stats::on_ray_cast_event(m::isfinite(work.triangle.dist) && work.triangle.dist<=traversal_range.max, 
                                 !m::isfinite(work.triangle.dist) && !V().contains(ray.propagate(traversal_range.max)),
                                 true,
                                 start,
                                 nodes);

    assert(work.triangle.dist == limits<length_t>::infinity() || traversal_range.contains(work.triangle.dist));
    return work.triangle.dist < limits<length_t>::infinity();
}


/**
 * ball traversal routines
 */

template <bool traverse_all=false>
inline bool gather_tris(const bvh8w_t* tree,
                        const ball_t &ball,
                        const std::uint32_t t0,
                        const std::uint32_t tcount,
                        const ads_t::intersect_opts_t& opts,
                        intersection_record_vec_work_t &record) noexcept {
    if constexpr (!traverse_all) {
        bool found_intersection = false;
        for (auto t=0ul; t<tcount; t+=8) {
            const auto tidx = t0+t;
            const auto tris = load_tri_cluster_8w(tree, tidx);
            
            const auto intrs = intersect::test_ball_tri(
                    ball,
                    tris.a,
                    tris.b,
                    tris.c,
                    tris.n);
            for (auto i=0; i<m::min<int>(8,tcount-t); ++i) {
                if (intrs.read(i)!=0) {
                    found_intersection = true;
                    record.triangles.emplace_back(intersection_work_tri_t{
                        .tuid = (tuid_t)(tidx+i),
                    });
                }
            }
        }

        return found_intersection;
    } else {
        for (std::size_t t=0; t<tcount; ++t) {
            const auto tuid = (tuid_t)(t0+t);
            record.triangles.emplace_back(intersection_work_tri_t{
                .tuid = tuid,
            });
        }
        return true;
    }
}

inline bool traverse_all(const bvh8w_t* tree,
                         const ball_t &ball,
                         const int32_t ptr,
                         const ads_t::intersect_opts_t& opts,
                         intersection_record_vec_work_t &record) noexcept {
    std::uint32_t t0, tcount;

    assert(!bvh8w::is_ptr_empty(ptr));
    if (bvh8w::is_ptr_leaf(ptr)) {
        const auto& leaf = tree->leaf_node(bvh8w::leaf_node_ptr(ptr));
        t0 = leaf.tris_ptr;
        tcount = leaf.count;
    } else {
        const auto& n = tree->node(bvh8w::child_node_ptr(ptr));
        t0 = n.tris_start;
        tcount = n.tris_count;
    }

    return gather_tris<true>(tree, ball, t0, tcount,
                             opts, record);
}

bool traverse(const bvh8w_t *tree,
              const ball_t &ball,
              const std::int32_t ptr,
              intersection_record_vec_work_t &record,
              const ads_t::intersect_opts_t& opts) noexcept {
    constexpr auto stack_size = 128;
    int32_t stack[stack_size];
    int s=1;

    stack[0] = tree->root_ptr();
    for (;s>0;) {
        if (bvh8w::is_ptr_leaf(stack[s-1])) {
            const auto& leaf = tree->leaf_node(bvh8w::leaf_node_ptr(stack[s-1]));
            // find intersecting tris in leaf (if exists)
            gather_tris(tree, ball,
                        leaf.tris_ptr, leaf.count,
                        opts, record);
            --s;
        }
        else {
            // traverse node
            const auto& n = tree->node(bvh8w::child_node_ptr(stack[s-1]));
            --s;

            const bool should_traverse_as_leaf = n.tris_count <= ray_traversal_treat_node_as_leaf_if_triangle_count_lt;
            if (should_traverse_as_leaf) {
                // find closest intersecting tri in subtree (if exists)
                gather_tris(tree, ball,
                            n.tris_start, n.tris_count,
                            opts, record);

                continue;
            }

            const auto& aabbs8w = bvh8w::node_aabbs(n);
            const auto r = intersect::test_ball_aabb(ball, aabbs8w.min, aabbs8w.max);
            const auto intersects = r.intersects_mask.to_bitmask();

            // gather intersected children
            for (int i=0;i<8;++i) {
                const auto& ptr = n.child_ptrs[i];
                if (bvh8w::is_ptr_empty(ptr))
                    continue;

                if (intersects[i]) {
#ifndef RELEASE
                    if (s==stack_size) std::exit(99); // stack overflow
#endif
                    stack[s++] = ptr;
                }
            }
        }
    }

    return !record.triangles.empty();
}

intersection_record_t bvh8w_t::intersect(const ball_t &ball,
                                         const intersect_opts_t& opts) const noexcept {
    intersection_record_vec_work_t work;
    ::traverse(this, ball, root_ptr(), work, opts);

    return ball_work_to_intersection_record(*this, work, opts);
}


scene::element::info_t bvh8w_t::description() const {
    using namespace scene::element;
    return {
        .cls = "ADS",
        .type = "bvh8w",
        .attribs = {
            { "triangles", attributes::make_scalar(tris.size()) },
            { "nodes",     attributes::make_scalar(nodes.size()) },
            { "occupancy", attributes::make_scalar(occupancy) },
            { "max depth", attributes::make_scalar(max_depth) },
        }
    };
}
