/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <vector>

#include <wt/mesh/mesh.hpp>
#include <wt/mesh/cylinder.hpp>

using namespace wt;
using namespace wt::mesh;


mesh_t cylinder_t::create(const std::string& shape_id, 
                          const wt_context_t& ctx,
                          const pqvec3_t& p0, const pqvec3_t& p1, length_t r, 
                          const transform_d_t& world,
                          const std::size_t phi_tessellation) {
    const auto v = p1-p0;
    const auto len = m::length(v);
    const auto transform = 
        transform_t::translate(p0) * 
        transform_t::to_frame(frame_t::build_orthogonal_frame(m::normalize(v)));

    const auto d = m::two_pi/f_t(phi_tessellation) * u::ang::rad;
    const std::size_t verts = 2*phi_tessellation;
    std::vector<pqvec3_t> vertices;
    std::vector<dir3_t> normals;
    std::vector<vec2_t> texcoords;
    std::vector<mesh_t::tri_indices_t> tris;
    vertices.resize(verts);
    normals.resize(verts, dir3_t{ 0,0,1 });
    texcoords.resize(verts);
    tris.resize(verts);

    for (size_t phiidx=0, tid=0, vid=0; phiidx<phi_tessellation; ++phiidx, vid+=2, tid+=2) {
        const auto idx0 = (mesh_t::tidx_t)vid;
        const auto idx1 = idx0+1;
        const auto idx2 = (vid+2) % verts;
        const auto idx3 = idx2+1;

        const auto phi = phiidx*d;
        const auto s = m::sin(phi);
        const auto c = m::cos(phi);
        vertices[vid]    = transform(pqvec3_t{ c*r, s*r, 0*u::m }, transform_point);
        vertices[vid+1]  = transform(pqvec3_t{ c*r, s*r, len }, transform_point);
        normals[vid+1]   = normals[vid] = transform(dir3_t{ c,s,0 });
        texcoords[vid]   = vec2_t{ phiidx / f_t(phi_tessellation), 0 };
        texcoords[vid+1] = vec2_t{ phiidx / f_t(phi_tessellation), 1 };

        assert(m::isfinite(vertices[vid]) &&
               m::isfinite(vertices[vid+1]));

        tris[tid].idx[0] = idx0;
        tris[tid].idx[1] = idx2;
        tris[tid].idx[2] = idx1;
        tris[tid+1].idx[0] = idx1;
        tris[tid+1].idx[1] = idx2;
        tris[tid+1].idx[2] = idx3;
    }

    return {
        shape_id,
        world,
        std::move(vertices), 
        std::move(normals), 
        std::move(texcoords), 
        tris,
        ctx
    };
}
