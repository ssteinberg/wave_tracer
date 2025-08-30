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

using namespace wt;
using namespace wt::mesh;


mesh_t icosahedron_t::create(const std::string& shape_id, 
                             const wt_context_t& ctx,
                             const pqvec3_t& centre, length_t radius,
                             const transform_d_t& world) {
    const auto transform = transform_t::translate(centre);

    std::vector<pqvec3_t> vertices;
    std::vector<dir3_t> normals;
    std::vector<vec2_t> texcoords;
    std::vector<mesh_t::tri_indices_t> tris;
    
    const auto a = f_t(1);
    const auto b = f_t(1) / m::golden_ratio;

    vertices.reserve(12);
    normals.reserve(12);
    texcoords.reserve(12);
    tris.reserve(20);

    vertices.emplace_back(vec3_t{ 0, b, -a } * u::m);
    vertices.emplace_back(vec3_t{ b, a, 0 } * u::m);
    vertices.emplace_back(vec3_t{ -b, a, 0 } * u::m);
    vertices.emplace_back(vec3_t{ 0, b, a } * u::m);
    vertices.emplace_back(vec3_t{ 0, -b, a } * u::m);
    vertices.emplace_back(vec3_t{ -a, 0, b } * u::m);
    vertices.emplace_back(vec3_t{ 0, -b, -a } * u::m);
    vertices.emplace_back(vec3_t{ a, 0, -b } * u::m);
    vertices.emplace_back(vec3_t{ a, 0, b } * u::m);
    vertices.emplace_back(vec3_t{ -a, 0, -b } * u::m);
    vertices.emplace_back(vec3_t{ b, -a, 0 } * u::m);
    vertices.emplace_back(vec3_t{ -b, -a, 0 } * u::m);

    // project
    for (auto& v : vertices) {
        const auto n = m::normalize(v);
        v = transform(radius * n, transform_point);

        normals.emplace_back(transform(n));
        texcoords.emplace_back(
            u::to_rad(m::atan2(n.z,n.x)) * m::inv_two_pi,
            u::to_rad(m::asin(n.y) / m::pi) + f_t(.5)
        );
    }

    tris.emplace_back(mesh_t::tri_indices_t{ 2, 1, 0 });
    tris.emplace_back(mesh_t::tri_indices_t{ 1, 2, 3 });
    tris.emplace_back(mesh_t::tri_indices_t{ 5, 4, 3 });
    tris.emplace_back(mesh_t::tri_indices_t{ 4, 8, 3 });
    tris.emplace_back(mesh_t::tri_indices_t{ 7, 6, 0 });
    tris.emplace_back(mesh_t::tri_indices_t{ 6, 9, 0 });
    tris.emplace_back(mesh_t::tri_indices_t{ 11, 10, 4 });
    tris.emplace_back(mesh_t::tri_indices_t{ 10, 11, 6 });
    tris.emplace_back(mesh_t::tri_indices_t{ 9, 5, 2 });
    tris.emplace_back(mesh_t::tri_indices_t{ 5, 9, 11 });
    tris.emplace_back(mesh_t::tri_indices_t{ 8, 7, 1 });
    tris.emplace_back(mesh_t::tri_indices_t{ 7, 8, 10 });
    tris.emplace_back(mesh_t::tri_indices_t{ 2, 5, 3 });
    tris.emplace_back(mesh_t::tri_indices_t{ 8, 1, 3 });
    tris.emplace_back(mesh_t::tri_indices_t{ 9, 2, 0 });
    tris.emplace_back(mesh_t::tri_indices_t{ 1, 7, 0 });
    tris.emplace_back(mesh_t::tri_indices_t{ 11, 9, 6 });
    tris.emplace_back(mesh_t::tri_indices_t{ 7, 10, 6 });
    tris.emplace_back(mesh_t::tri_indices_t{ 5, 11, 4 });
    tris.emplace_back(mesh_t::tri_indices_t{ 10, 8, 4 });

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
