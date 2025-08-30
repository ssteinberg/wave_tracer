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

#include <wt/math/transform/transform.hpp>
#include <wt/mesh/mesh.hpp>
#include <wt/mesh/obj_loader.hpp>

#include <wt/math/common.hpp>

#include <wt/util/logger/logger.hpp>


#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include <tiny_obj_loader.h>

using namespace wt;
using namespace wt::mesh;

mesh_t obj_loader::load_obj(const std::string& shape_id,
                            const wt_context_t& ctx,
                            const std::filesystem::path &path, 
                            const std::optional<std::string>& objects_for_mtl,
                            bool face_normals,
                            const transform_d_t& transform,
                            length_t scale) {
    std::vector<vec3_t> vertices;
    std::vector<dir3_t> normals;
    std::vector<vec2_t> texcoords;
    std::vector<mesh_t::tri_indices_t> tris;

    {
        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = path.parent_path().string();

        tinyobj::ObjReader reader;
        if (!reader.ParseFromFile(path.c_str(), reader_config)) {
            if (!reader.Error().empty())
                logger::cerr(verbosity_e::important) << "(obj loader) Loading failed: " << reader.Error() << '\n';
            throw std::runtime_error("(obj loader) obj loading failed");
        }
        if (!reader.Warning().empty())
            logger::cwarn(verbosity_e::info) << "(obj loader) " << reader.Warning() << '\n';

        [[maybe_unused]] const auto& attrib = reader.GetAttrib();
        [[maybe_unused]] const auto& shapes = reader.GetShapes();
        [[maybe_unused]] const auto& materials = reader.GetMaterials();

        bool warn_polygonal_faces = false;
        bool has_normals = true, has_uv = true;
        for (const auto& shape : shapes) {
            std::size_t index_offset = 0;
            for (std::size_t f=0; f<shape.mesh.num_face_vertices.size(); ++f) {
                const auto fv = std::size_t(shape.mesh.num_face_vertices[f]);
                index_offset += fv;

                const auto matid = shape.mesh.material_ids[f];
                assert(matid==-1 || matid<materials.size());
                if (objects_for_mtl) {
                    // skip faces with undesired material
                    if ((matid==-1 && objects_for_mtl->empty()) || 
                        (matid>=0 && materials[matid].name!=*objects_for_mtl))
                        continue;
                }

                // ignore non-triangular faces
                if (fv!=3) {
                    warn_polygonal_faces = true;
                    continue;
                }

                for (std::size_t v=0; v<fv; ++v) {
                    const auto idx = shape.mesh.indices[index_offset-fv+v];

                    if (vertices.empty()) {
                        // initialize which vertex attributes are contained
                        has_normals = !face_normals && idx.normal_index>=0;
                        has_uv = idx.texcoord_index>=0;
                    }
                    
                    if (has_normals != idx.normal_index>=0 && !face_normals)
                        throw std::runtime_error("(obj loader) OBJ file is missing normal data from some vertices. This is unsupported.");
                    if (has_uv != idx.texcoord_index>=0)
                        throw std::runtime_error("(obj loader) OBJ file is missing uv data from some vertices. This is unsupported.");

                    vertices.emplace_back(
                        attrib.vertices[3*std::size_t(idx.vertex_index)+0],
                        attrib.vertices[3*std::size_t(idx.vertex_index)+1],
                        attrib.vertices[3*std::size_t(idx.vertex_index)+2]
                    );
                    if (has_normals && idx.normal_index>=0) {
                        const auto n = vec3_t{
                                attrib.normals[3*std::size_t(idx.normal_index)+0],
                                attrib.normals[3*std::size_t(idx.normal_index)+1],
                                attrib.normals[3*std::size_t(idx.normal_index)+2]
                            };
                        normals.emplace_back(m::length2(n)>0 ? m::normalize(n) : dir3_t{ 0,0,1 });
                    } 
                    if (has_uv && idx.texcoord_index>=0) {
                        texcoords.emplace_back(
                            attrib.texcoords[2*std::size_t(idx.texcoord_index)+0],
                            attrib.texcoords[2*std::size_t(idx.texcoord_index)+1]
                        );
                    }
                }

                const auto i = mesh_t::tidx_t(vertices.size()-3);
                tris.push_back(mesh_t::tri_indices_t{ .idx={ i,i+1,i+2 } });
            }
        }

        if (warn_polygonal_faces)
            logger::cerr(verbosity_e::important) << "(obj loader) Encountered unsupported non-triangular faces" << '\n';
    }

    if (vertices.empty() && objects_for_mtl) {
        logger::cwarn() << "(obj loader) No faces found for supplied 'mtl'" << '\n';
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
