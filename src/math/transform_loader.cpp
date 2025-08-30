/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <sstream>

#include <wt/scene/loader/loader.hpp>
#include <wt/scene/loader/node_readers.hpp>
#include <wt/util/format/parse.hpp>
#include <wt/util/format/parse_quantity.hpp>

#include <wt/math/transform/transform.hpp>
#include <wt/math/transform/transform_loader.hpp>

#include <wt/util/logger/logger.hpp>
#include <wt/util/format/utils.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;


template <FloatingPoint Fp>
inline auto parse_transform_matrix4(const std::string& str) {
    mat4<Fp> m{};
    std::stringstream ss(str);
    std::string expr;
    for (std::size_t r=0;r<4;++r)
    for (std::size_t c=0;c<4;++c) {
        if (c==3 && r!=3) {
            // last row is translation
            length_t l;
            ss >> l;
            m[c][r] = u::to_m(l);
        }
        else {
            std::getline(ss, expr, ',');
            m[c][r] = stonum_strict<Fp>(format::trim(expr));
        }

        if (ss.peek() == ',')
            ss.ignore();
    }
    return m;
}

template <FloatingPoint Fp>
inline auto loader(const scene::loader::node_t& node, scene::loader::loader_t* loader) {
    using R = transform_generic_t<Fp>;
    using v3 = R::v3_t;
    using pqv3 = R::pqv3_t;
    using angle_t = R::angle_type;

    // Either a "lookat" can be provided
    const auto& lookat_children = node.children("lookat");
    if (!lookat_children.empty()) {
        const auto& lookat_node = *lookat_children.front();

        pqv3 origin,target;
        v3 up;
        try {
            origin = lookat_node.has_attrib("origin") ?
                parse_pqvec3<Fp>(lookat_node["origin"]) : 
                pqv3::zero();
            target = lookat_node.has_attrib("target") ?
                parse_pqvec3<Fp>(lookat_node["target"]) : 
                pqv3{ 0*u::m,0*u::m,1*u::m };
            up = lookat_node.has_attrib("up") ?
                parse_vec3<Fp>(lookat_node["up"]) :
                v3{ frame_t::build_orthogonal_frame(dir3_t{ m::normalize(target-origin) }).t };
        } catch(const std::format_error& exp) {
            throw scene_loading_exception_t("(transform loader) " + std::string{ exp.what() }, lookat_node);
        }

        if (1-m::abs(m::dot(up,m::normalize(target-origin)))<Fp(1e-5))
            throw scene_loading_exception_t("(transform loader) degenerate 'lookat' transform", lookat_node);

        for (auto& item : node.children_view()) {
            if (item.name() != "lookat")
                logger::cwarn()
                    << loader->node_description(item)
                    << "(transform loader) Unqueried node type " << item.name() << " (lookat is exclusive)" << '\n';
        }
        if (lookat_children.size()>1)
            logger::cerr()
                << loader->node_description(node)
                << "(transform loader) multiple lookat nodes defined" << '\n';
        
        return R::lookat(origin, target, up);
    }

    // ... or a sequence of other transforms
    R transform;
    for (auto& item : node.children_view()) {
    try {
        if (item.name() == "matrix") {
            transform = R{ parse_transform_matrix4<Fp>(item["value"]) } * transform;
        }
        else if (item.name() == "rotate") {
            const auto axis = scene::loader::read_vec_attribute<v3>(item);
            const auto angle = stoq_strict<angle_t>(item["angle"]);
            transform = R::rotate(axis,angle) * transform;
        }
        else if (item.name() == "translate") {
            const auto translation = scene::loader::read_vec_attribute<pqv3>(item);
            transform = R::translate(translation) * transform;
        }
        else if (item.name() == "scale") {
            const auto scale = scene::loader::read_vec_attribute<v3>(item, v3{ 1,1,1, });
            transform = R::scale(scale) * transform;
        }
        else 
            logger::cwarn()
                << loader->node_description(item)
                << "(transform loader) Unqueried node type " << item.name() << " (\"" << node.name() << "\")" << '\n';
    } catch(const std::runtime_error& exp) {
        throw scene_loading_exception_t("(transform loader) " + std::string{ exp.what() }, item);
    }
    }

    return transform;
}

transform_f_t wt::load_transform_sfp(const scene::loader::node_t& node, scene::loader::loader_t* sloader) {
    return loader<float>(node, sloader);
}

transform_d_t wt::load_transform_dfp(const scene::loader::node_t& node, scene::loader::loader_t* sloader) {
    return loader<double>(node, sloader);
}
