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
#include <wt/mesh/prism.hpp>

using namespace wt;
using namespace wt::mesh;


mesh_t prism_t::create(const std::string& shape_id,
                       const wt_context_t& ctx,
                       const transform_d_t& world,
                       length_t length, length_t height, angle_t angle) {
    constexpr auto tricount = 8;
    constexpr auto verts = 18;
    static const vec3_t pos[] = {
        { -.5, 0, -.5 }, { .5, 0, -.5 },  { 0,1,-.5 },
        { -.5, 0, .5 },  { 0,1,.5 },      { .5, 0, .5 },
        { -.5, 0, .5 },  { -.5, 0, -.5 }, { 0,1,.5 },    { 0,1,-.5 },
        { .5, 0, -.5 },  { .5, 0, .5 },   { 0,1,-.5 },   { 0,1,.5 },
        { -.5, 0, .5 },  { -.5, 0, -.5 }, { .5, 0, .5 }, { .5, 0, -.5 },
    };
    static const vec2_t uvs[] = {
        {0,0}, {1,0}, {.5,.5},
        {0,0}, {.5,.5}, {1,0},
        {0,0}, {1,0}, {0,1}, {1,1},
        {0,0}, {1,0}, {0,1}, {1,1},
        {0,0}, {1,0}, {0,1}, {1,1},
    };
    static const mesh_t::tidx_t tids[][3] = {
        {0, 2, 1}, {3, 5, 4}, 
        {6, 8, 7}, {9, 7, 8},
        {10, 12, 11}, {13, 11, 12},
        {14, 15, 16}, {17, 16, 15},
    };

    std::vector<pqvec3_t> vertices;
    std::vector<vec2_t> texcoords;
    std::vector<mesh_t::tri_indices_t> tris;
    vertices.resize(verts);
    texcoords.resize(verts);
    tris.resize(tricount);

    const auto xlen = height * m::tan(angle/2);
    const auto scale = pqvec3_t{ xlen, height, length };

    for (std::size_t i=0; i<verts; ++i) {
        vertices[i]  = pos[i] * scale;
        texcoords[i] = uvs[i];
    }
    for (std::size_t i=0; i<tricount; ++i) {
        tris[i].idx[0] = tids[i][0];
        tris[i].idx[1] = tids[i][1];
        tris[i].idx[2] = tids[i][2];
    }

    return {
        shape_id,
        world,
        std::move(vertices), 
        {}, 
        std::move(texcoords), 
        tris,
        ctx
    };
}
