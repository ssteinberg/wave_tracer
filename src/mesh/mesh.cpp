/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <vector>

#include <cassert>

#include <wt/mesh/mesh.hpp>

#include <wt/math/common.hpp>
#include <wt/math/frame.hpp>
#include <wt/math/transform/transform.hpp>
#include <wt/math/encoded_normal.hpp>
#include <wt/math/util.hpp>

#include <wt/scene/element/attributes.hpp>

#include <wt/math/glm.hpp>
#include <glm/gtx/hash.hpp>

using namespace wt;
using namespace wt::mesh;


inline auto tris_from_indices(const std::string& shape_id,
                              const transform_d_t& to_world,
                              std::vector<pqvec3_t> vertices,
                              std::vector<dir3_t> normals,
                              std::vector<vec2_t> texcoords,
                              const std::vector<mesh_t::tri_indices_t>& indices) {
    std::vector<triangle_t> tris;
    tris.reserve(indices.size());

    for (const auto& ind : indices) {
        // transform vertices using doubles
        auto a = to_world((pqvec3d_t)vertices[ind.idx[0]], transform_point);
        auto b = to_world((pqvec3d_t)vertices[ind.idx[1]], transform_point);
        auto c = to_world((pqvec3d_t)vertices[ind.idx[2]], transform_point);

        // texcoords
        std::optional<std::array<vec2_t, 3>> uvs;
        if (!texcoords.empty()) {
            uvs = { texcoords[ind.idx[0]],texcoords[ind.idx[1]],texcoords[ind.idx[2]] };
            assert(m::isfinite((*uvs)[0]) && m::isfinite((*uvs)[1]) && m::isfinite((*uvs)[2]));
        }

        // compute geometric normal
        const auto geo_n = util::tri_face_normal(a,b,c);
        if (!geo_n) {
            // skip degenerate triangles (0 area, no normal)
            continue;
        }
        auto gn = (dir3_t)*geo_n;

        std::array<encoded_normal_t, 3> ns;
        if (!normals.empty()) {
            // renormalize in double precision and transform
            auto n1 = (dir3_t)to_world(m::normalize((vec3d_t)normals[ind.idx[0]]));
            auto n2 = (dir3_t)to_world(m::normalize((vec3d_t)normals[ind.idx[1]]));
            auto n3 = (dir3_t)to_world(m::normalize((vec3d_t)normals[ind.idx[2]]));

            // flip winding order when normals are on other side of geometric normal
            if (m::dot(n1,gn)<0 && m::dot(n2,gn)<0 && m::dot(n3,gn)<0) {
                std::swap(a,b);
                if (uvs)
                    std::swap((*uvs)[0],(*uvs)[1]);
                std::swap(n1,n2);
                gn = -gn;
            }

            ns = { 
                encoded_normal_t{ n1 },
                encoded_normal_t{ n2 },
                encoded_normal_t{ n3 }
            };
        }
        else {
            const auto enc_geo_n = encoded_normal_t{ gn };
            ns = { enc_geo_n,enc_geo_n,enc_geo_n };
        }

        tris.emplace_back(triangle_t{
            .p = { 
                (pqvec3_t)a,
                (pqvec3_t)b,
                (pqvec3_t)c
            },
            .geo_n = gn,
            .n = ns,
            .uv = uvs,
        });
    }

    return tris;
}


mesh_t::mesh_t(std::vector<triangle_t> tris)
    : tris(std::move(tris)),
      aabb(aabb_t::null())
{
    compute_aabb();
    compute_tangent_frames();
}

mesh_t::mesh_t(const std::string& shape_id,
               const transform_d_t& to_world,
               std::vector<pqvec3_t> vertices,
               std::vector<dir3_t> normals,
               std::vector<vec2_t> texcoords,
               const std::vector<tri_indices_t>& indices,
               const wt_context_t& ctx) 
    : mesh_t(tris_from_indices(
                    shape_id, 
                    to_world,
                    std::move(vertices), 
                    std::move(normals), 
                    std::move(texcoords), 
                    indices))
{}

void mesh_t::compute_aabb() noexcept {
    aabb = aabb_t::null();
    for (const auto& t : tris)
        aabb |= aabb_t::from_points(t.p[0],t.p[1],t.p[2]);
}

void mesh_t::compute_tangent_frames() noexcept {
    for (auto& t : tris) {
        const auto& p0  = t.p[0];
        const auto& p1  = t.p[1];
        const auto& p2  = t.p[2];
        const auto& uv0 = !t.uv ? vec2_t{ 0,0 } : (*t.uv)[0];
        const auto& uv1 = !t.uv ? vec2_t{ 0,0 } : (*t.uv)[1];
        const auto& uv2 = !t.uv ? vec2_t{ 0,0 } : (*t.uv)[2];
        t.tangent_frame = surface_differentials_for_triangle(p0, p1, p2, uv0, uv1, uv2);
    }
}


scene::element::info_t mesh_t::description() const {
    using namespace scene::element;
    return {
        .id = "",
        .cls = "mesh",
        .type = "mesh",
        .attribs = {
            { "triangles", attributes::make_scalar(tris.size()) },
        }
    };
}
