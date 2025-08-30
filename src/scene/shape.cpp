/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <algorithm>
#include <string>
#include <algorithm>
#include <numeric>
#include <optional>

#include <filesystem>

#include <wt/wt_context.hpp>

#include <wt/util/unreachable.hpp>
#include <wt/util/logger/logger.hpp>

#include <wt/scene/shape.hpp>
#include <wt/scene/loader/node.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/emitter/area.hpp>

#include <wt/math/transform/transform_loader.hpp>
#include <wt/mesh/rectangle.hpp>
#include <wt/mesh/cube.hpp>
#include <wt/mesh/prism.hpp>
#include <wt/mesh/sphere.hpp>
#include <wt/mesh/icosahedron.hpp>
#include <wt/mesh/cylinder.hpp>
#include <wt/mesh/lens.hpp>
#include <wt/mesh/ply_loader.hpp>
#include <wt/mesh/obj_loader.hpp>

#include <wt/scene/loader/node_readers.hpp>

using namespace wt;
using namespace mesh;


inline auto construct_triangle_surface_area_distribution(const mesh_t& mesh) {
    assert(mesh.get_tris().size());

    std::vector<area_t> surface_areas;
    surface_areas.resize(mesh.get_tris().size());
    std::transform(mesh.get_tris().cbegin(), mesh.get_tris().cend(), surface_areas.begin(), 
                   [&](const auto& tri) { return mesh_t::triangle_surface_area(tri); });

    const area_t surface_area = std::reduce(surface_areas.cbegin(), surface_areas.cend());

    return shape_t::triangle_sampling_data_t{
        .triangle_surface_area_distribution = discrete_distribution_t<area_t>{ 
            std::move(surface_areas),
            [](const auto a) { return u::to_m2(a); }
        },
        .surface_area = surface_area,
        .recp_surface_area = 1 / surface_area,
    };
}

shape_t::shape_t(std::string id,
                 std::shared_ptr<bsdf_t> bsdf,
                 std::shared_ptr<emitter_t> emitter, 
                 mesh_t mesh) 
    : scene_element_t(std::move(id)),
      bsdf(std::move(bsdf)), 
      emitter(std::move(emitter)), 
      mesh(std::move(mesh)),
      sampling_data(construct_triangle_surface_area_distribution(this->mesh))
{}

position_sample_t shape_t::sample_position(sampler::sampler_t& sampler) const noexcept {
    const auto r = sampler.r3();

    // sample a triangle w.r.t. to surface area
    const auto idx = sampling_data.triangle_surface_area_distribution.icdf(r.z);
    // sample a point on the triangle
    const auto bary = sampler.uniform_triangle(vec2_t{ r });

    const auto intersection = 
        intersection_surface_t(this, 
                               std::uint32_t(idx), 
                               bary);

    return position_sample_t{
        .p = intersection.wp,
        .ppd = sampling_data.recp_surface_area,
        .surface = intersection,
    };
}


scene::element::info_t shape_t::description() const {
    using namespace scene::element;

    auto mesh_info = mesh.description();
    auto mesh_desc = std::move(mesh_info.attribs);
    mesh_desc.emplace("cls", attributes::make_string(mesh_info.cls));

    auto ret = info_for_scene_element(*this, "shape", {
        { "mesh", attributes::make_map(std::move(mesh_desc)) },
            { "surface area", attributes::make_scalar(get_surface_area()) },
    });

    if (bsdf)    ret.attribs.emplace("bsdf", attributes::make_element(bsdf.get()));
    if (emitter) ret.attribs.emplace("emitter", attributes::make_element(emitter.get()));

    return ret;
}


std::shared_ptr<shape_t> shape_t::load(std::string id, 
                                       scene::loader::loader_t* loader, 
                                       const scene::loader::node_t& node, 
                                       const wt::wt_context_t &context) {
    std::vector<const scene::loader::node_t*> consumed_attributes;

    // first read to_world transform
    transform_d_t to_world;
    for (auto& item : node.children_view()) {
        if (scene::loader::load_transform(item, "to_world", to_world, loader)) {
            consumed_attributes.emplace_back(&item);
            break;
        }
    }

    // ... load and create the mesh...
    auto mesh = load_mesh(node, id, to_world, context, consumed_attributes);

    // discard empty shapes
    if (mesh.get_tris().empty()) {
        wt::logger::cwarn(verbosity_e::important)
            << loader->node_description(node)
            << "(mesh) shape '" << id << "' contains no geometry and has been discarded." << '\n';
        return nullptr;
    }

    // ... finally, if the above is OK, load rest of attributes

    std::shared_ptr<bsdf_t> bsdf;
    std::shared_ptr<emitter::emitter_t> emitter;
    bool flip_normals = false;

    for (auto& item : node.children_view()) {
        if (scene::loader::read_attribute(item, "flip_normals", flip_normals) ||
            scene::loader::load_scene_element(item, bsdf, loader, context) ||
            scene::loader::load_scene_element(item, emitter, loader, context))
            consumed_attributes.emplace_back(&item);
    }

    // flip normals if requested
    if (flip_normals)
        mesh.flip_normals();

    if (!bsdf)
        throw scene_loading_exception_t("(shape loader) no bsdf found", node);
    if (emitter && !emitter->is_area_emitter())
        throw scene_loading_exception_t("(shape loader) emitter must be an area emitter", node);

    // create shape, and assign shape emitter, if any
    auto shape = std::make_shared<shape_t>(std::move(id), 
                                           std::move(bsdf),
                                           std::move(emitter),
                                           std::move(mesh));
    if (shape->emitter)
        dynamic_pointer_cast<emitter::area_t>(shape->emitter)->set_shape(context, shape.get());

    
    // warn about unused attributes
    std::ranges::sort(consumed_attributes);
    for (auto& item : node.children_view()) {
        if (!std::ranges::binary_search(consumed_attributes, &item))
            logger::cwarn()
                << loader->node_description(item)
                << "(shape loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    }


    return shape;
}

mesh_t shape_t::load_mesh(const scene::loader::node_t& node, 
                          const std::string& shape_id,
                          const transform_d_t& to_world,
                          const wt::wt_context_t &context,
                          std::vector<const scene::loader::node_t*>& consumed_attributes) {
    using namespace mesh;
    
    const std::string& type = node["type"];

    if (type=="rectangle") {
        std::optional<length_t> len;
        std::optional<pqvec3_t> p, x, y;
        std::size_t tessellation = 1;

        for (auto& item : node.children_view()) {
        try {
            if (scene::loader::read_attribute(item, "length", len) ||
                scene::loader::read_attribute(item, "p", p) ||
                scene::loader::read_attribute(item, "x", x) ||
                scene::loader::read_attribute(item, "y", y) ||
                scene::loader::read_attribute(item, "tessellation", tessellation))
                consumed_attributes.emplace_back(&item);
        } catch(const std::format_error& exp) {
            throw scene_loading_exception_t("(shape loader) " + std::string{ exp.what() }, item);
        }
        }

        // either point and edges must be specified, or length, or no attribute
        const auto test = int(!!p) + int(!!x) + int(!!y);
        if ((test!=0 && test!=3) || (test==3 && len))
            throw scene_loading_exception_t("(shape loader) either a point 'p' and edges 'x' and 'y' must be provided, or edge length 'length', or none of these attributes", node);
        if (!x && !y && !p && !len)
            len = 2*u::m;

        if (tessellation==0)
            throw scene_loading_exception_t("(shape loader) 'tessellation' must be a positive integer", node);

        return len ?
            rectangle_t::create(shape_id, context, to_world, *len, tessellation) :
            rectangle_t::create(shape_id, context, *p, *x, *y, to_world, tessellation);
    }

    if (type=="cube") {
        length_t len = 2*u::m;

        for (auto& item : node.children_view()) {
        try {
            if (scene::loader::read_attribute(item, "length", len))
                consumed_attributes.emplace_back(&item);
        } catch(const std::format_error& exp) {
            throw scene_loading_exception_t("(shape loader) " + std::string{ exp.what() }, item);
        }
        }

        return cube_t::create(shape_id, context, to_world, len);
    }

    if (type=="prism") {
        length_t len  = 1*u::m, height=1*u::m;
        angle_t angle = m::pi/2 * u::ang::rad;

        for (auto& item : node.children_view()) {
        try {
            if (scene::loader::read_attribute(item, "length", len) ||
                scene::loader::read_attribute(item, "height", height) ||
                scene::loader::read_attribute(item, "angle", angle))
                consumed_attributes.emplace_back(&item);
        } catch(const std::format_error& exp) {
            throw scene_loading_exception_t("(shape loader) " + std::string{ exp.what() }, item);
        }
        }

        return prism_t::create(shape_id, context, to_world, len, height, angle);
    }

    if (type=="sphere") {
        pqvec3_t centre{};
        length_t radius = 1*u::mm;
        std::size_t tessellation = 32;

        for (auto& item : node.children_view()) {
        try {
            if (scene::loader::read_attribute(item, "center", centre) ||
                scene::loader::read_attribute(item, "radius", radius) ||
                scene::loader::read_attribute(item, "tessellation", tessellation))
                consumed_attributes.emplace_back(&item);
        } catch(const std::format_error& exp) {
            throw scene_loading_exception_t("(shape loader) " + std::string{ exp.what() }, item);
        }
        }

        if (tessellation==0)
            throw scene_loading_exception_t("(shape loader) 'tessellation' must be a positive integer", node);

        return sphere_t::create(shape_id, context, centre, radius, to_world, tessellation);
    }

    if (type=="icosahedron") {
        pqvec3_t centre{};
        length_t radius = 1*u::mm;

        for (auto& item : node.children_view()) {
        try {
            if (scene::loader::read_attribute(item, "center", centre) ||
                scene::loader::read_attribute(item, "radius", radius))
                consumed_attributes.emplace_back(&item);
        } catch(const std::format_error& exp) {
            throw scene_loading_exception_t("(shape loader) " + std::string{ exp.what() }, item);
        }
        }

        return icosahedron_t::create(shape_id, context, centre, radius, to_world);
    }

    if (type=="cylinder") {
        pqvec3_t p0{},p1{};
        length_t radius =  1*u::mm;
        std::size_t tessellation = 32;

        for (auto& item : node.children_view()) {
        try {
            if (scene::loader::read_attribute(item, "p0", p0) ||
                scene::loader::read_attribute(item, "p1", p1) ||
                scene::loader::read_attribute(item, "radius", radius) ||
                scene::loader::read_attribute(item, "tessellation", tessellation))
                consumed_attributes.emplace_back(&item);
        } catch(const std::format_error& exp) {
            throw scene_loading_exception_t("(shape loader) " + std::string{ exp.what() }, item);
        }
        }

        if (tessellation==0)
            throw scene_loading_exception_t("(shape loader) 'tessellation' must be a positive integer", node);

        return cylinder_t::create(shape_id, context, p0, p1, radius, to_world, tessellation);
    }

    if (type=="lens") {
        pqvec3_t centre{};
        length_t radius = 1*u::mm;
        f_t R1 = 0;
        f_t R2 = 0;
        length_t thickness = 0*u::m;
        std::size_t tessellation = 50;

        for (auto& item : node.children_view()) {
        try {
            if (scene::loader::read_attribute(item, "center", centre) ||
                scene::loader::read_attribute(item, "radius", radius) ||
                scene::loader::read_attribute(item, "R1", R1) ||
                scene::loader::read_attribute(item, "R2", R2) ||
                scene::loader::read_attribute(item, "thickness", thickness) ||
                scene::loader::read_attribute(item, "tessellation", tessellation))
                consumed_attributes.emplace_back(&item);
        } catch(const std::format_error& exp) {
            throw scene_loading_exception_t("(shape loader) " + std::string{ exp.what() }, item);
        }
        }

        if (tessellation==0)
            throw scene_loading_exception_t("(shape loader) 'tessellation' must be a positive integer", node);

        return lens_t::create(shape_id, context, centre, radius,
                              { .thickness=thickness, .R1=R1, .R2=R2 },
                              to_world,
                              tessellation);
    }

    if (type=="ply" || type=="obj") {
        bool face_normals = false;
        std::optional<std::filesystem::path> path;
        std::optional<std::string> objmtl;
        length_t scale = context.default_scale_for_imported_mesh_positions;

        for (auto& item : node.children_view()) {
        try {
            if (scene::loader::read_attribute(item, "face_normals", face_normals) ||
                scene::loader::read_attribute(item, path) ||
                scene::loader::read_attribute(item, "mtl", objmtl) ||
                scene::loader::read_attribute(item, "scale", scale))
                consumed_attributes.emplace_back(&item);
        } catch(const std::format_error& exp) {
            throw scene_loading_exception_t("(shape loader) " + std::string{ exp.what() }, item);
        }
        }

        if (!path)
            throw scene_loading_exception_t("(shape loader) path must be provided for a ply shape", node);

        const auto resolved_path = context.resolve_path(*path);
        if (!resolved_path)
            throw scene_loading_exception_t("(shape loader) path '" + path->string() + "' not found.", node);

        const auto mesh_id = shape_id.length()==0 ? "shape_" + path->filename().string() : shape_id;

        if (type=="ply") {
            if (objmtl)
                throw scene_loading_exception_t("(shape loader) ply shape do not support 'mtl'", node);
            return ply_loader::load_ply(mesh_id, context, 
                                        *resolved_path, 
                                        face_normals, to_world, scale);
        }
        if (type=="obj") {
            return obj_loader::load_obj(mesh_id, context, 
                                        *resolved_path, objmtl, 
                                        face_normals, to_world, scale);
        }

        unreachable();
    }
    
    throw scene_loading_exception_t("(shape loader) unrecognized shape type", node);
}
