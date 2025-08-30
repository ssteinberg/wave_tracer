/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 * Authors:  Umut Emre, Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#include <wt/ads/bvh_constructor.hpp>
#include <wt/ads/traversal_common.hpp>


// SAH parameters for costs of traversal / insertion
// For cone tracing, build very deep trees: subtrees contained in a cone get traversed as a leaf.
#define C_INT 100
#define C_TRAV 1

#define NO_CUSTOM_GEOMETRY
#define NO_INDEXED_GEOMETRY

#ifdef _NO_DBL_SUPPORT
#define NO_DOUBLE_PRECISION_SUPPORT
#define TBVH_USE_SINGLES
#endif

#define BVHBINS 128
#define HQBVHBINS 256
#define MAXHQBINS 1024
static constexpr auto tbvh_optimize_iterations = 32;

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#define TINYBVH_IMPLEMENTATION
#include <tiny_bvh.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif


using namespace wt;
using namespace wt::ads;
using namespace wt::ads::construction;


inline bvh::node_t tbvh_to_bvh_recursive(
#ifdef TBVH_USE_SINGLES
        const tinybvh::BVH::BVHNode* tbvh_nodes,
        const tinybvh::BVH::BVHNode& tnode,
        const std::uint32_t* tbvh_tidx,
#else
        const tinybvh::BVH_Double::BVHNode* tbvh_nodes,
        const tinybvh::BVH_Double::BVHNode& tnode,
        const std::uint64_t* tbvh_tidx,
#endif
        std::vector<bvh::node_cluster_t>& node_clusters,
        const std::vector<tri_t>& all_tris,
        std::vector<tri_t>& bvhtris) noexcept {
    bvh::node_t n;
    n.aabb = aabb_t{ vec3_t{ tnode.aabbMin.x, tnode.aabbMin.y, tnode.aabbMin.z } * u::m,
                     vec3_t{ tnode.aabbMax.x, tnode.aabbMax.y, tnode.aabbMax.z } * u::m };

    assert(n.aabb.isfinite());


    if (tnode.isLeaf()) {
        assert(tnode.triCount>0);

        n.is_leaf = true;
        n.tri_count = tnode.triCount;
        n.tris_offset = (idx_t)bvhtris.size();

        for (idx_t tidx = 0; tidx<tnode.triCount; ++tidx)
            bvhtris.emplace_back(all_tris[tbvh_tidx[tidx + tnode.leftFirst]]);
    } else {
        n.is_leaf = false;
        n.tri_count = 0;
        n.children_cluster_offset = node_clusters.size();
        node_clusters.emplace_back();

        const auto& tchild1 = tbvh_nodes[tnode.leftFirst];
        const auto& tchild2 = tbvh_nodes[tnode.leftFirst + 1];

        node_clusters[n.children_cluster_offset].nodes[0] = 
            tbvh_to_bvh_recursive(tbvh_nodes, tchild1, tbvh_tidx,
                                  node_clusters, all_tris, bvhtris);
        node_clusters[n.children_cluster_offset].nodes[1] = 
            tbvh_to_bvh_recursive(tbvh_nodes, tchild2, tbvh_tidx,
                                  node_clusters, all_tris, bvhtris);
    }

    return n;
}

void tree_dfs_write_triangle_ptrs(std::vector<bvh::node_cluster_t>& node_clusters,
                                  bvh::node_t& node) {
    if (node.is_leaf) return;

    auto &l = node_clusters[node.children_cluster_offset].nodes[0];
    auto &r = node_clusters[node.children_cluster_offset].nodes[1];

    tree_dfs_write_triangle_ptrs(node_clusters,l);
    tree_dfs_write_triangle_ptrs(node_clusters,r);

    [[maybe_unused]] std::uint32_t tcount = l.tri_count + r.tri_count;
    const auto trl = range_t<std::uint32_t, range_inclusiveness_e::left_inclusive>{ l.tris_offset, l.tris_offset+l.tri_count };
    const auto trr = range_t<std::uint32_t, range_inclusiveness_e::left_inclusive>{ r.tris_offset, r.tris_offset+r.tri_count };
    const auto trange = trl | trr;

    assert(trange.length()>0);
    assert(trange.length() == tcount);
    assert(!trl.overlaps(trr));

    node.tris_offset = trange.min;
    node.tri_count = trange.length();
}

bvh_constructor_t::bvh_constructor_t(
        std::vector<std::shared_ptr<shape_t>> objs,
        const wt::wt_context_t& ctx,
        progress_track_t& pt)
{
    constexpr auto build_pb = .35f;
    constexpr auto optimize_pb = .95f;

    {
        // extract tris from shapes and initialize bvh_primitives
        std::vector<tri_t> all_tris;
        for (idx_t objid = 0; objid < objs.size(); ++objid) {
            const auto& obj = objs[objid];

            const auto& mesh = obj->get_mesh();
            const auto& mesh_tris = mesh.get_tris();

            for (mesh::mesh_t::tidx_t triid = 0; triid < mesh_tris.size(); ++triid) {
                const mesh::triangle_t& tri = mesh_tris[triid];

                all_tris.emplace_back(tri_t{
                    .a = tri.p[0],
                    .b = tri.p[1],
                    .c = tri.p[2],
                    .n = tri.geo_n,
                    .shape_idx = objid,
                    .shape_tri_idx = triid
                });
            }
        }

        if (all_tris.empty())
            throw std::runtime_error("(bvh_constructor) no triangles found!");

        // build BVH using tinybvh

        using tbvh_t = 
#ifdef TBVH_USE_SINGLES
            tinybvh::BVH;
#else
            tinybvh::BVH_Double;
#endif
        tbvh_t tbvh;

        {
            pt.set_status("tiny_bvh build()");
            f_t prog = 0;
            tinybvh::progress_tracker = [&](f_t p) {
                if (p>prog+f_t(.02)) {
                    pt.set_progress(p * build_pb);
                    prog = p;
                }
            };

#ifdef TBVH_USE_SINGLES
            {
                std::vector<tinybvh::bvhvec4> tbvhtris;
                tbvhtris.reserve(all_tris.size()*3);
                for (const auto& t : all_tris) {
                    tbvhtris.emplace_back(u::to_m(t.a.x), u::to_m(t.a.y), u::to_m(t.a.z), 0);
                    tbvhtris.emplace_back(u::to_m(t.b.x), u::to_m(t.b.y), u::to_m(t.b.z), 0);
                    tbvhtris.emplace_back(u::to_m(t.c.x), u::to_m(t.c.y), u::to_m(t.c.z), 0);
                }

                tbvh.BuildAVX(tbvhtris.data(), tbvhtris.size()/3);
            }
#else
            {
                std::vector<tinybvh::bvhdbl3> tbvhtris;
                tbvhtris.reserve(all_tris.size()*3);
                for (const auto& t : all_tris) {
                    tbvhtris.emplace_back((double)u::to_m(t.a.x), (double)u::to_m(t.a.y), (double)u::to_m(t.a.z));
                    tbvhtris.emplace_back((double)u::to_m(t.b.x), (double)u::to_m(t.b.y), (double)u::to_m(t.b.z));
                    tbvhtris.emplace_back((double)u::to_m(t.c.x), (double)u::to_m(t.c.y), (double)u::to_m(t.c.z));
                }

                tbvh.Build(tbvhtris.data(), tbvhtris.size()/3);
            }
#endif

            pt.set_progress(build_pb);
            pt.set_status("tiny_bvh optimize()");
            tinybvh::progress_tracker = [&](f_t p) {
                pt.set_progress(p * (optimize_pb-build_pb) + build_pb);
            };

#ifdef TBVH_USE_SINGLES
            {
                tinybvh::BVH_Verbose tmp;
                tmp.ConvertFrom(tbvh);
                tmp.Optimize(tbvh_optimize_iterations);
                tbvh.ConvertFrom(tmp);
            }
#endif

            pt.set_progress(optimize_pb);
        }

        const auto sah_cost = tbvh.SAHCost();

        // ... and re-encode from tinybvh to ours

        pt.set_status("encoding binary BVH");

        std::vector<bvh::node_cluster_t> node_clusters;
        std::vector<tri_t> bvhtris;
#ifdef TBVH_USE_SINGLES
        node_clusters.reserve(tbvh.NodeCount()/2);
#else
        node_clusters.reserve(tbvh.usedNodes/2);
#endif
        bvhtris.reserve(all_tris.size());

        node_clusters.emplace_back();
        node_clusters.front().nodes[0] = 
            tbvh_to_bvh_recursive(tbvh.bvhNode, tbvh.bvhNode[0], tbvh.primIdx,
                                  node_clusters, all_tris, bvhtris);

        // write triangle references from internal nodes
        tree_dfs_write_triangle_ptrs(node_clusters, node_clusters[0].nodes[0]);

        node_clusters.shrink_to_fit();
        bvhtris.shrink_to_fit();
        bvh = std::make_unique<bvh_t>(std::move(node_clusters), std::move(bvhtris), sah_cost);
    }
    
    // done building
    pt.set_progress(1);
}
