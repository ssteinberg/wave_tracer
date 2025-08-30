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
#include <wt/mesh/cube.hpp>

using namespace wt;
using namespace wt::mesh;


mesh_t cube_t::create(const std::string& shape_id, const wt_context_t& ctx, const transform_d_t& transform, length_t scale) {
    constexpr auto tricount = 12;
    constexpr auto verts = 24;
    static const vec3_t pos[] = {
        {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}, {-1, -1, -1}, 
        {1, 1, -1}, {-1, 1, -1}, {-1, 1, 1}, {1, 1, 1}, 
        {1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {1, -1, 1}, 
        {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}, {-1, -1, 1}, 
        {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}, {-1, -1, -1}, 
        {1, 1, -1}, {1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}
    };
    static const dir3_t ns[] = {
        {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, 
        {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, 
        {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, 
        {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, 
        {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, 
        {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}
    };
    static const vec2_t uvs[] = {
        {0, 1}, {1, 1}, {1, 0}, {0, 0}, 
        {0, 1}, {1, 1}, {1, 0}, {0, 0}, 
        {0, 1}, {1, 1}, {1, 0}, {0, 0}, 
        {0, 1}, {1, 1}, {1, 0}, {0, 0},
        {0, 1}, {1, 1}, {1, 0}, {0, 0}, 
        {0, 1}, {1, 1}, {1, 0}, {0, 0}
    };
    static const mesh_t::tidx_t tids[][3] = {
        {0, 1, 2}, {3, 0, 2}, 
        {4, 5, 6}, {7, 4, 6}, 
        {8, 9, 10}, {11, 8, 10}, 
        {12, 13, 14}, {15, 12, 14}, 
        {16, 17, 18}, {19, 16, 18}, 
        {20, 21, 22}, {23, 20, 22}
    };

    std::vector<pqvec3_t> vertices;
    std::vector<dir3_t> normals;
    std::vector<vec2_t> texcoords;
    std::vector<mesh_t::tri_indices_t> tris;
    vertices.resize(verts);
    normals.resize(verts, dir3_t{ 0,0,1 });
    texcoords.resize(verts);
    tris.resize(tricount);

    for (std::size_t i=0; i<verts; ++i) {
        vertices[i]  = pos[i] * scale/2;
        normals[i]   = ns[i];
        texcoords[i] = uvs[i];

        assert(m::isfinite(vertices[i]) &&
               m::isfinite(texcoords[i]));
    }
    for (std::size_t i=0; i<tricount; ++i) {
        tris[i].idx[0] = tids[i][0];
        tris[i].idx[1] = tids[i][1];
        tris[i].idx[2] = tids[i][2];
    }

    return {
        shape_id,
        transform,
        std::move(vertices), 
        std::move(normals), 
        std::move(texcoords), 
        tris,
        ctx
    };
}
