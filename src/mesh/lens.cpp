/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <vector>

#include <cassert>

#include <wt/mesh/mesh.hpp>
#include <wt/mesh/lens.hpp>

using namespace wt;
using namespace wt::mesh;


mesh_t lens_t::create(const std::string& shape_id, 
                      const wt_context_t& ctx,
                      const pqvec3_t& centre, length_t radius, 
                      const lens_properties_t& lens,
                      const transform_d_t& world,
                      const std::size_t tessellation) {
    const auto transform = transform_t::translate(centre);

    std::vector<pqvec3_t> vertices;
    std::vector<dir3_t> normals;
    std::vector<vec2_t> texcoords;
    std::vector<mesh_t::tri_indices_t> tris;

    const auto R1 = radius / lens.R1;
    const auto R2 = radius / lens.R2;

    const auto x1 = m::isfinite(R1) ? 
                    +m::sign(R1) * m::sqrt(m::sqr(R1) - m::sqr(radius)) : 
                    0*u::m;
    const auto x2 = m::isfinite(R2) ? 
                    -m::sign(R2) * m::sqrt(m::sqr(R2) - m::sqr(radius)) : 
                    0*u::m;

    const auto Lf = pqvec3_t{ x1, 0*u::m, 0*u::m };
    const auto Rf = pqvec3_t{ x2, 0*u::m, 0*u::m };

    auto ET = x1-x2 - (m::isfinite(R1)?R1:0*u::m) - (m::isfinite(R2)?R2:0*u::m) + lens.thickness;
    // avoid faces of a double-concave lens touching each other
    if (lens.thickness==0*u::m && lens.R1<=0 && lens.R2<=0)
        ET += radius/1000;

    vertices.reserve(m::sqr(tessellation)*2+2);
    normals.reserve(m::sqr(tessellation)*2+2);
    texcoords.reserve(m::sqr(tessellation)*2+2);
    tris.reserve(m::sqr(tessellation)*2);

    const auto L_tess = m::isfinite(R1) ? tessellation : 1;
    const auto R_tess = m::isfinite(R2) ? tessellation : 1;

    // left face
    const auto L_start_idx = vertices.size();
    vertices.emplace_back(Lf + pqvec3_t{ -(m::isfinite(R1)?R1:0*u::m),0*u::m,0*u::m });
    normals.emplace_back(-1,0,0);
    texcoords.emplace_back(0,0);
    for (int i=0;i<L_tess;++i) {
        const auto h = radius * m::min<f_t>(1,m::pow((i+1)/f_t(L_tess), f_t(.8)));
        for (int j=0;j<tessellation;++j) {
            const auto phi = j/f_t(tessellation) * m::two_pi * u::ang::rad;
            const auto cp = vec3_t{ 0, m::cos(phi), m::sin(phi) } * h;

            pqvec3_t p; dir3_t n = { -1,0,0 };
            if (m::isfinite(R1)) {
                n = m::normalize(cp-Lf);
                if (R1<0*u::m) n=-n;
                p = Lf + n*R1;
            } else {
                p = cp;
            }

            assert(m::isfinite(p));

            vertices.emplace_back(p);
            normals.emplace_back(n);
            texcoords.emplace_back((i+1)/f_t(tessellation+1), j/f_t(tessellation));
        }
    }

    // right face
    const auto R_start_idx = vertices.size();
    vertices.emplace_back(Rf + pqvec3_t{ (m::isfinite(R2)?R2:0*u::m)+ET,0*u::m,0*u::m });
    normals.emplace_back(1,0,0);
    texcoords.emplace_back(0,0);
    for (int i=0;i<R_tess;++i) {
        const auto h = radius * m::min<f_t>(1,m::pow((i+1)/f_t(R_tess), f_t(.8)));
        for (int j=0;j<tessellation;++j) {
            const auto phi = j/f_t(tessellation) * m::two_pi * u::ang::rad;
            const auto cp = vec3_t{ 0, m::cos(phi), m::sin(phi) } * h;

            pqvec3_t p; dir3_t n = { 1,0,0 };
            if (m::isfinite(R2)) {
                n = m::normalize(cp-Rf);
                if (R2<0*u::m) n=-n;
                p = Rf + n*R2 + pqvec3_t{ ET,0*u::m,0*u::m };
            } else {
                p = cp + pqvec3_t{ ET,0*u::m,0*u::m };
            }

            assert(m::isfinite(p));

            vertices.emplace_back(p);
            normals.emplace_back(n);
            texcoords.emplace_back((i+1)/f_t(tessellation+1), j/f_t(tessellation));
        }
    }

    // edge
    const auto E_start_idx = vertices.size();
    if (ET>0*u::m) {
        for (int j=0;j<tessellation;++j) {
            const auto phi = j/f_t(tessellation) * m::two_pi * u::ang::rad;
            const auto n  = dir3_t{ 0, m::cos(phi), m::sin(phi) };
            const auto cp = n * radius;
            
            vertices.emplace_back(cp);
            vertices.emplace_back(cp + pqvec3_t{ ET,0*u::m,0*u::m });
            normals.emplace_back(n);
            normals.emplace_back(n);
            texcoords.emplace_back(0, j/f_t(tessellation));
            texcoords.emplace_back(1, j/f_t(tessellation));

            assert(m::isfinite(cp));
        }
    }

    // construct tris
    for (int i=0;i<L_tess;++i) {
        for (int j=0;j<tessellation;++j) {
            const auto previ0 = (i-1)*tessellation + (j>0?j-1:tessellation-1);
            const auto previ1 = (i-1)*tessellation + j;
            const auto prev  = i*tessellation + (j>0?j-1:tessellation-1);
            if (i==0) {
                tris.emplace_back(mesh_t::tri_indices_t{ (std::uint32_t)(L_start_idx),
                                                         (std::uint32_t)(L_start_idx+1+j),
                                                         (std::uint32_t)(L_start_idx+1+prev) });
            } else {
                tris.emplace_back(mesh_t::tri_indices_t{ (std::uint32_t)(L_start_idx+1+previ0),
                                                         (std::uint32_t)(L_start_idx+1+previ1),
                                                         (std::uint32_t)(L_start_idx+1+prev) });
                tris.emplace_back(mesh_t::tri_indices_t{ (std::uint32_t)(L_start_idx+1+prev),
                                                         (std::uint32_t)(L_start_idx+1+previ1),
                                                         (std::uint32_t)(L_start_idx+1+i*tessellation+j) });
            }
        }
    }
    for (int i=0;i<R_tess;++i) {
        for (int j=0;j<tessellation;++j) {
            const auto previ0 = (i-1)*tessellation + (j>0?j-1:tessellation-1);
            const auto previ1 = (i-1)*tessellation + j;
            const auto prev  = i*tessellation + (j>0?j-1:tessellation-1);
            if (i==0) {
                tris.emplace_back(mesh_t::tri_indices_t{ (std::uint32_t)(R_start_idx),
                                                         (std::uint32_t)(R_start_idx+1+prev),
                                                         (std::uint32_t)(R_start_idx+1+j) });
            } else {
                tris.emplace_back(mesh_t::tri_indices_t{ (std::uint32_t)(R_start_idx+1+previ1),
                                                         (std::uint32_t)(R_start_idx+1+previ0),
                                                         (std::uint32_t)(R_start_idx+1+prev) });
                tris.emplace_back(mesh_t::tri_indices_t{ (std::uint32_t)(R_start_idx+1+previ1),
                                                         (std::uint32_t)(R_start_idx+1+prev),
                                                         (std::uint32_t)(R_start_idx+1+i*tessellation+j) });
            }
        }
    }
    if (ET>0*u::m) {
        for (int j=0;j<tessellation;++j) {
            const auto prev0 = (j>0?2*j-2:2*tessellation-2);
            const auto prev1 = prev0+1;
            tris.emplace_back(mesh_t::tri_indices_t{ (std::uint32_t)(E_start_idx+prev1),
                                                     (std::uint32_t)(E_start_idx+prev0),
                                                     (std::uint32_t)(E_start_idx+2*j) });
            tris.emplace_back(mesh_t::tri_indices_t{ (std::uint32_t)(E_start_idx+2*j+1),
                                                     (std::uint32_t)(E_start_idx+prev1),
                                                     (std::uint32_t)(E_start_idx+2*j) });
        }
    }

    for (auto& v : vertices)
        v = transform(v, transform_point);
    for (auto& n : normals)
        n = transform(n);

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
