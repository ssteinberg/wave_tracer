/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#include <list>
#include <vector>
#include <memory>

#include <wt/ads/bvh8w/bvh8w_constructor.hpp>
#include <wt/ads/bvh8w/common.hpp>
#include <wt/ads/edge_classification.hpp>
#include <wt/ads/bvh_constructor.hpp>
#include <wt/ads/traversal_common.hpp>

#include <wt/util/thread_pool/tpool.hpp>

using namespace wt;
using namespace wt::ads;
using namespace wt::ads::construction;


template <int depth>
inline void extract_bvh_nodes(const bvh_t::node_t& bvn,
                              const std::vector<bvh_t::node_cluster_t>& bvnc,
                              std::vector<const bvh_t::node_t*>& nodes,
                              int& nodes_consumed) noexcept {
    if (bvn.is_leaf || depth>0)
        ++nodes_consumed;
    if (bvn.is_leaf || depth==0) {
        nodes.emplace_back(&bvn);
        return;
    }

    if constexpr (depth>0) {
        extract_bvh_nodes<depth-1>(bvnc[bvn.children_cluster_offset].nodes[0], bvnc, nodes, nodes_consumed);
        extract_bvh_nodes<depth-1>(bvnc[bvn.children_cluster_offset].nodes[1], bvnc, nodes, nodes_consumed);
    }
}


struct work_item_t {
    const bvh_t::node_t* n;
};
struct build_result_t {
    bvh8w::node_t w8node{};

    std::vector<work_item_t> work_items;
    idx_t w8_idx;

    int bbvh_nodes_consumed=0;
};

inline auto build_node8w(idx_t w8_idx, 
                         const bvh::node_t& bvn,
                         const aabb_t& parent_aabb,
                         const std::vector<bvh_t::node_cluster_t>& bvnc,
                         const std::vector<tri_t>& tris,
                         progress_track_t& pt) {
    auto ret = build_result_t{ .w8_idx = w8_idx };

    // extract 3 levels of nodes from the BVH
    std::vector<const bvh_t::node_t*> bvh_child_nodes;
    bvh_child_nodes.reserve(8);
    extract_bvh_nodes<3>(bvn, bvnc, bvh_child_nodes, ret.bbvh_nodes_consumed);

    std::array<length_t,8> node_min_x, node_max_x, node_min_y, node_max_y, node_min_z, node_max_z;

    // encode
    for (auto c=0ul; c<bvh_child_nodes.size(); ++c) {
        const auto& n = bvh_child_nodes[c];
        node_min_x[c] = n->aabb.min.x;
        node_min_y[c] = n->aabb.min.y;
        node_min_z[c] = n->aabb.min.z;
        node_max_x[c] = n->aabb.max.x;
        node_max_y[c] = n->aabb.max.y;
        node_max_z[c] = n->aabb.max.z;

        // create child construction work item
        ret.work_items.push_back({ .n=n });
    }

    ret.w8node.min = pqvec3_w8_t{ node_min_x.data(), node_min_y.data(), node_min_z.data(), simd::unaligned_data };
    ret.w8node.max = pqvec3_w8_t{ node_max_x.data(), node_max_y.data(), node_max_z.data(), simd::unaligned_data };
    ret.w8node.tris_start = bvn.tris_offset;
    ret.w8node.tris_count = bvn.tri_count;

#ifdef DEBUG
    // sanity
    for (auto t = ret.w8node.tris_start; t<ret.w8node.tris_start+ret.w8node.tris_count; ++t) {
        const auto& tri = tris[t];
        assert(parent_aabb.contains<range_inclusiveness_e::inclusive>(tri.a));
        assert(parent_aabb.contains<range_inclusiveness_e::inclusive>(tri.b));
        assert(parent_aabb.contains<range_inclusiveness_e::inclusive>(tri.c));
    }
#endif

    return ret;
}

void calculate_occupancy(const bvh8w::node_t& n, const std::vector<bvh8w::node_t>& nodes,
                         std::size_t& filled_chld, std::size_t& potential_chld, std::size_t& max_depth, std::size_t depth=0) {
    potential_chld += 8;
    max_depth = std::max(max_depth, depth);
    for (const auto& ptr : n.child_ptrs) {
         if (!bvh8w::is_ptr_empty(ptr)) {
            ++filled_chld;
            if (bvh8w::is_ptr_child(ptr))
                calculate_occupancy(nodes[bvh8w::child_node_ptr(ptr)],
                                    nodes, filled_chld, potential_chld, max_depth, depth+1);
        }
    }
}

inline auto create_w8_tris(const std::vector<tri_t>& tris) {
    bvh8w_t::tris_vectorized_data_t d;
    const auto ds = tris.size()+7;
    d.ax.resize(ds);
    d.ay.resize(ds);
    d.az.resize(ds);
    d.bx.resize(ds);
    d.by.resize(ds);
    d.bz.resize(ds);
    d.cx.resize(ds);
    d.cy.resize(ds);
    d.cz.resize(ds);
    d.nx.resize(ds);
    d.ny.resize(ds);
    d.nz.resize(ds);

    for (auto t=0ul;t<tris.size();++t) {
        d.ax[t] = tris[t].a.x;
        d.ay[t] = tris[t].a.y;
        d.az[t] = tris[t].a.z;
        d.bx[t] = tris[t].b.x;
        d.by[t] = tris[t].b.y;
        d.bz[t] = tris[t].b.z;
        d.cx[t] = tris[t].c.x;
        d.cy[t] = tris[t].c.y;
        d.cz[t] = tris[t].c.z;
        d.nx[t] = tris[t].n.x;
        d.ny[t] = tris[t].n.y;
        d.nz[t] = tris[t].n.z;
    }

    return d;
}

bvh8w_constructor_t::bvh8w_constructor_t(
        std::vector<std::shared_ptr<shape_t>> objs,
        const wt::wt_context_t& ctx,
        std::optional<progress_callback_t> progress_callbacks)
{
    constexpr f_t pt_bvh_progress_portion = .7;
    constexpr f_t pt_8w_progress_portion  = .2;
    constexpr f_t pt_edge_finding_portion = 1 - pt_8w_progress_portion - pt_bvh_progress_portion;


    const auto start_timepoint = std::chrono::high_resolution_clock::now();
    pt.callbacks = std::move(progress_callbacks);


    // build plain binary BVH
    pt.proportion = pt_bvh_progress_portion;

    auto bvh_ctor = bvh_constructor_t{ std::move(objs), ctx, pt };
    auto bvh = std::move(bvh_ctor.bvh);

    if (!bvh)
        throw std::runtime_error("(BVH8w) BVH construction failed");

    const auto& bvhnc = bvh->node_clusters();


    // ... and encode an 8-wide BVH from it

    pt.start = pt_bvh_progress_portion;
    pt.proportion = pt_8w_progress_portion;
    pt.set_status("encoding 8-wide BVH");

    const auto total = bvhnc.size()*2;
    std::size_t completed = 0;

    std::vector<bvh8w::node_t> w8nodes;
    std::vector<bvh8w::leaf_node_t> w8_leaf_nodes;
    w8nodes.reserve(total/8);

    // create 8-wide clusters in parallel
    auto w8tris_future = std::async(std::launch::async, [&](){ return create_w8_tris(bvh->tree_tris); });

    // encode 8-wide tree
    std::list<std::future<build_result_t>> futures;
    const auto world = bvh->V();

    w8nodes.emplace_back();     // create root
    futures.emplace_back(ctx.threadpool->enqueue([&]() {
        return build_node8w(0, bvh->root(), world, bvhnc, bvh->tree_tris, pt);
    }));
    for (;!futures.empty();) {
        const auto ret = std::move(futures.front()).get();
        futures.pop_front();

        w8nodes[ret.w8_idx] = ret.w8node;
        for (auto c=0ul; c<ret.work_items.size(); ++c) {
            const auto& wi = ret.work_items[c];
            const auto& n  = *wi.n;

            if (n.is_leaf) {
                w8_leaf_nodes.emplace_back(bvh8w::leaf_node_t{ .tris_ptr = n.tris_offset, .count = n.tri_count });
                if (w8_leaf_nodes.size()>limits<std::int32_t>::max())       // panic
                    throw std::runtime_error("(BVH8w) Too many nodes!");

                w8nodes[ret.w8_idx].child_ptrs[c] = -(std::int32_t)(w8_leaf_nodes.size());  // leaf ptrs have set signs
            } else {
                const auto cidx = (idx_t)w8nodes.size();
                w8nodes.emplace_back();     // create new node

                if (cidx>limits<std::int32_t>::max())       // panic
                    throw std::runtime_error("(BVH8w) Too many nodes!");

                w8nodes[ret.w8_idx].child_ptrs[c] = (std::int32_t)cidx+1;   // child ptrs have unset signs

                const auto paabb = wi.n->aabb;
                futures.emplace_back(ctx.threadpool->enqueue([w8idx=cidx, &n, paabb, &bvhnc, &bvh, this]() {
                    return build_node8w(w8idx, n, paabb, bvhnc, bvh->tree_tris, pt);
                }));
            }
        }

        completed += ret.bbvh_nodes_consumed;
        pt.set_progress(completed / f_t(total));
    }
    assert(completed==total-1);

    // calculate occupancy
    std::size_t filled_chld = 0, potential_chld = 0, max_depth = 0;
    calculate_occupancy(w8nodes[0], w8nodes, filled_chld, potential_chld, max_depth);
    f_t occupancy = filled_chld / f_t(potential_chld);

    // finish tri clusters
    auto w8tris = std::move(w8tris_future).get();


    // create bvh8w
    w8nodes.shrink_to_fit();
    w8_leaf_nodes.shrink_to_fit();
    bvh8w = std::make_unique<bvh8w_t>(std::move(w8nodes), std::move(w8_leaf_nodes), 
                                      std::move(w8tris), std::move(bvh->tree_tris), 
                                      world, bvh->get_sah_cost(), occupancy, max_depth);


    // find edges
    pt.start = pt_bvh_progress_portion + pt_8w_progress_portion;
    pt.proportion = pt_edge_finding_portion;
    pt.set_status("finding edges");

    bvh8w->edges = find_edges(bvh8w.get(), bvh8w->tris, ctx, pt);
    

    // done building
    this->build_time = std::chrono::high_resolution_clock::now() - start_timepoint;
    pt.set_status("");
    pt.complete();
}
