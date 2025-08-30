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
#include <wt/mesh/icosahedron.hpp>
#include <wt/mesh/sphere.hpp>

using namespace wt;
using namespace wt::mesh;


mesh_t sphere_t::create(const std::string& shape_id, 
                        const wt_context_t& ctx,
                        const pqvec3_t& centre, length_t r, 
                        const transform_d_t& world,
                        const std::size_t tessellation) {
    std::vector<pqvec3_t> vertices;
    std::vector<dir3_t> normals;
    std::vector<vec2_t> texcoords;
    std::vector<mesh_t::tri_indices_t> tris;

    // start with an icosahedron
    const auto icosahedron = icosahedron_t::create("temp", ctx, vec3_t{ 0,0,0 }*u::m, 1*u::m, {});
    const auto& icstris = icosahedron.get_tris();

    // tessellate
    std::function<void(const pqvec3_t&, const pqvec3_t&, const pqvec3_t&, const vec2_t, const vec2_t, const vec2_t, int)> tessellate_and_add;
    tessellate_and_add = [&](const pqvec3_t& p0, const pqvec3_t& p1, const pqvec3_t& p2, 
                             const vec2_t uv0, const vec2_t uv1, const vec2_t uv2,
                             int recursion) {
        if (recursion==0) {
            const auto n0 = m::normalize(p0);
            const auto n1 = m::normalize(p1);
            const auto n2 = m::normalize(p2);
            
            vertices.emplace_back(r*n0 + centre);
            vertices.emplace_back(r*n1 + centre);
            vertices.emplace_back(r*n2 + centre);
            normals.emplace_back(n0);
            normals.emplace_back(n1);
            normals.emplace_back(n2);
            texcoords.emplace_back(uv0);
            texcoords.emplace_back(uv1);
            texcoords.emplace_back(uv2);

            const auto vidx = (std::uint32_t)vertices.size()-3;
            tris.emplace_back(mesh_t::tri_indices_t{ vidx,vidx+1,vidx+2 });

            return;
        }

        // subdivide
        const auto p01  = (p0+p1) / f_t(2);
        const auto uv01 = (uv0+uv1) / f_t(2);
        const auto p02  = (p0+p2) / f_t(2);
        const auto uv02 = (uv0+uv2) / f_t(2);
        const auto p12  = (p1+p2) / f_t(2);
        const auto uv12 = (uv1+uv2) / f_t(2);
        tessellate_and_add(p0,  p01,  p02,
                           uv0, uv01, uv02,
                           recursion-1);
        tessellate_and_add(p01,  p1,  p12,
                           uv01, uv1, uv12,
                           recursion-1);
        tessellate_and_add(p01,  p12,  p02,
                           uv01, uv12, uv02,
                           recursion-1);
        tessellate_and_add(p02,  p12,  p2,
                           uv02, uv12, uv2,
                           recursion-1);
    };

    const auto recursion = (std::size_t)(m::max<f_t>(0,m::log2(f_t(tessellation)/3)) + f_t(.5));
    for (const auto& t : icstris)
        tessellate_and_add(t.p[0], t.p[1], t.p[2], (*t.uv)[0], (*t.uv)[1], (*t.uv)[2], recursion);

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
