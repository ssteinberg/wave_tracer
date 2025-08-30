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
#include <wt/mesh/rectangle.hpp>

using namespace wt;
using namespace wt::mesh;

mesh_t rectangle_t::create(const std::string &shape_id,
                           const wt_context_t& ctx,
                           const pqvec3_t& p, 
                           const pqvec3_t& x,
                           const pqvec3_t& y, 
                           const transform_d_t& transform,
                           std::uint32_t tessellation) {
    std::vector<pqvec3_t> vertices;
    std::vector<vec2_t> texcoords;
    std::vector<mesh_t::tri_indices_t> tris;

    const auto recpt = f_t(1) / tessellation;

    vertices.reserve(4*m::sqr(tessellation));
    texcoords.reserve(4*m::sqr(tessellation));
    tris.reserve(2*m::sqr(tessellation));

    for (std::uint32_t ix=0; ix<tessellation; ++ix)
    for (std::uint32_t iy=0; iy<tessellation; ++iy) {
        const auto tidx = vertices.size();
        const auto u0 = (ix+0)*recpt;
        const auto v0 = (iy+0)*recpt;
        const auto u1 = ix+1==tessellation?1:(ix+1)*recpt;
        const auto v1 = iy+1==tessellation?1:(iy+1)*recpt;

        vertices.push_back(p + u0*x + v0*y);
        vertices.push_back(p + u1*x + v0*y);
        vertices.push_back(p + u1*x + v1*y);
        vertices.push_back(p + u0*x + v1*y);

        texcoords.emplace_back(u0,v0);
        texcoords.emplace_back(u1,v0);
        texcoords.emplace_back(u1,v1);
        texcoords.emplace_back(u0,v1);

        tris.emplace_back();
        tris.back().idx[0] = tidx + 0;
        tris.back().idx[1] = tidx + 1;
        tris.back().idx[2] = tidx + 2;

        tris.emplace_back();
        tris.back().idx[0] = tidx + 2;
        tris.back().idx[1] = tidx + 3;
        tris.back().idx[2] = tidx + 0;
    }

    return {
        shape_id,
        transform,
        std::move(vertices), 
        {}, 
        std::move(texcoords), 
        tris,
        ctx
    };
}

mesh_t rectangle_t::create(const std::string& shape_id,
                           const wt_context_t& ctx,
                           const transform_d_t& transform, 
                           length_t scale, std::uint32_t tessellation) {
    constexpr auto x = vec3_t{  1, 0,0 };
    constexpr auto y = vec3_t{  0, 1,0 };

    return create(shape_id,
                  ctx,
                  -(x+y) * scale/2,
                  x * scale,
                  y * scale,
                  transform,
                  tessellation);
}
