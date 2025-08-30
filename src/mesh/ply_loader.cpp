/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <vector>
#include <stdexcept>

#include <wt/mesh/mesh.hpp>
#include <wt/mesh/ply_loader.hpp>

#include <wt/math/common.hpp>

#include <miniply.h>

using namespace wt;
using namespace wt::mesh;

mesh_t ply_loader::load_ply(const std::string& shape_id,
                            const wt_context_t& ctx,
                            const std::filesystem::path &path, 
                            bool face_normals,
                            const transform_d_t& transform,
                            length_t scale) {
    std::vector<vec3_t> vertices;
    std::vector<dir3_t> normals;
    std::vector<vec2_t> texcoords;
    std::vector<mesh_t::tri_indices_t> tris;

    {
        miniply::PLYReader reader(path.c_str());
        if (!reader.valid())
            throw std::runtime_error("(ply loader) ply loading failed");

        const miniply::PLYPropertyType ply_type = std::is_same_v<f_t,float> ? miniply::PLYPropertyType::Float : 
                                                  std::is_same_v<f_t,double> ? miniply::PLYPropertyType::Double :
                                                  miniply::PLYPropertyType::None;
        if (ply_type==miniply::PLYPropertyType::None)
            throw std::runtime_error("(ply loader) float type not implemented");

        uint32_t indexes[3];
        bool gotVerts = false, gotFaces = false;
        while (reader.has_element()) {
            if (reader.element_is(miniply::kPLYVertexElement) && reader.load_element() && reader.find_pos(indexes)) {
                const std::size_t vert_count = reader.num_rows();

                vertices.resize(vert_count);
                reader.extract_properties(indexes, 3, ply_type, &vertices[0]);

                if (!face_normals && reader.find_normal(indexes)) {
                    normals.resize(vert_count, dir3_t{ 0,0,1 });
                    reader.extract_properties(indexes, 3, ply_type, &normals[0]);
                }

                if (reader.find_texcoord(indexes)) {
                    texcoords.resize(vert_count);
                    reader.extract_properties(indexes, 2, ply_type, &texcoords[0]);
                }

                gotVerts = true;
            }
            
            if (reader.element_is(miniply::kPLYFaceElement) && reader.load_element() && reader.find_indices(indexes)) {
                if (reader.requires_triangulation(indexes[0]))
                    throw std::runtime_error("(ply loader) triangulation not supported");

                std::vector<int> indices;
                const std::size_t num_tris = reader.num_rows();
                indices.resize(num_tris*3);
                reader.extract_list_property(indexes[0], miniply::PLYPropertyType::Int, &indices[0]);

                tris.resize(num_tris);
                for (std::size_t t=0;t<num_tris;++t) {
                    tris[t].idx[0] = indices[t*3+0];
                    tris[t].idx[1] = indices[t*3+1];
                    tris[t].idx[2] = indices[t*3+2];
                }

                gotFaces = true;
            }

            if (gotVerts && gotFaces)
                break;
            reader.next_element();
        }

        if (!gotVerts || !gotFaces) 
            throw std::runtime_error("(ply loader) bad PLY: failed reading vertices or faces");
    }

    std::vector<pqvec3_t> qvertices;
    qvertices.reserve(vertices.size());
    for (const auto& v : vertices)
        qvertices.emplace_back(v*scale);

    mesh_t mesh = mesh_t(shape_id,
                         transform,
                         std::move(qvertices), 
                         std::move(normals), 
                         std::move(texcoords), 
                         tris,
                         ctx);

    return mesh;
}
