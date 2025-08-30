/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <string>
#include <filesystem>
#include <optional>
#include <utility>
#include <concepts>

#include <wt/math/type_traits.hpp>
#include <wt/util/concepts.hpp>

#include <wt/spectrum/spectrum.hpp>
#include <wt/texture/texture.hpp>
#include <wt/texture/complex_constant.hpp>
#include <wt/texture/constant.hpp>
#include <wt/util/math_expression.hpp>
#include <wt/math/transform/transform_loader.hpp>

#include <wt/util/format/parse.hpp>
#include <wt/util/format/parse_quantity.hpp>
#include <wt/util/format/enum.hpp>

#include "node.hpp"
#include "loader.hpp"


namespace wt::scene::loader {


/* node data parsing: basic types */

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::string& out) {
    if (node.name()=="string" &&
        node["name"]==name) {
        out = node["value"];
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<std::string>& out) {
    if (node.name()=="string" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = node["value"];
        return true;
    }
    return false;
}

template <Enum E>
inline bool read_enum_attribute(const node_t& node, 
                                const std::string& name, 
                                E& out) {
    if (node.name()=="string" &&
        node["name"]==name) {
        const auto val = format::parse_enum<E>(node["value"]);
        if (!val)
            throw std::format_error("Unrecognized value in node \"" + name + "\"");
        out = *val;
        return true;
    }
    return false;
}
template <Enum E>
inline bool read_enum_attribute(const node_t& node, 
                                const std::string& name, 
                                std::optional<E>& out) {
    if (node.name()=="string" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        const auto val = format::parse_enum<E>(node["value"]);
        if (!val)
            throw std::format_error("Unrecognized value in node \"" + name + "\"");
        out = *val;
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           bool& out) {
    if (node.name()=="boolean" &&
        node["name"]==name) {
        out = stob_strict(node["value"]);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<bool>& out) {
    if (node.name()=="boolean" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = stob_strict(node["value"]);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           f_t& out) {
    if (node.name()=="float" &&
        node["name"]==name) {
        out = stof_strict(node["value"]);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<f_t>& out) {
    if (node.name()=="float" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = stof_strict(node["value"]);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::int16_t& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        out = stoi_strict(node["value"]);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<std::int16_t>& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = stoi_strict(node["value"]);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           int& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        out = stoi_strict(node["value"]);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<int>& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = stoi_strict(node["value"]);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           long long& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        out = stoll_strict(node["value"]);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<long long>& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = stoll_strict(node["value"]);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::uint16_t& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        out = stoul_strict(node["value"]);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<std::uint16_t>& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = stoul_strict(node["value"]);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::uint32_t& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        out = stoul_strict(node["value"]);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<std::uint32_t>& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = stoul_strict(node["value"]);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           unsigned long& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        out = stoul_strict(node["value"]);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<unsigned long>& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = stoul_strict(node["value"]);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           unsigned long long& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        out = stoull_strict(node["value"]);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<unsigned long long>& out) {
    if (node.name()=="integer" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = stoull_strict(node["value"]);
        return true;
    }
    return false;
}


template <Quantity Q>
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           Q& out) {
    if (node.name()=="quantity" &&
        node["name"]==name) {
        out = stoq_strict<Q>(node["value"]);
        return true;
    }
    return false;
}
template <Quantity Q>
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<Q>& out) {
    if (node.name()=="quantity" &&
        node["name"]==name) {
        out = stoq_strict<Q>(node["value"]);
        return true;
    }
    return false;
}

template <QuantityPoint Q>
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           Q& out) {
    if (node.name()=="quantity" &&
        node["name"]==name) {
        out = stoq_strict<Q>(node["value"]);
        return true;
    }
    return false;
}
template <QuantityPoint Q>
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<Q>& out) {
    if (node.name()=="quantity" &&
        node["name"]==name) {
        out = stoq_strict<Q>(node["value"]);
        return true;
    }
    return false;
}


template <typename V>
inline V read_vec_attribute(const node_t& node, const std::optional<V> default_value = {}) {
    constexpr auto elements = element_count_v<V>;
    using T = vector_element_type_t<V>;

    const auto reader = [](const auto& v) {
        if constexpr (QuantityVector<V>) return stoq_strict<T>(v);
        else                             return stonum_strict<T>(v);
    };
    const auto reader_opt = [](const auto& v, const auto opt) {
        if (v[0]=='\0') return opt;
        if constexpr (QuantityVector<V>) return stoq_strict<T>(v);
        else                             return stonum_strict<T>(v);
    };

    auto val = V{};

    const auto& value_attribute = node["value"];
    if (!value_attribute.empty()) {
        // check for conflicting attributes
        for (const auto& attr : node.attributes()) {
            const auto& attrn = attr.first;
            if (attrn!="value" && attrn!="id" && attrn!="name")
                throw std::format_error("Unqueried attribute " + attr.second + " (node \"" + node.name() + "\")");
        }

        std::string l;
        std::stringstream ss{ value_attribute };
        auto i=0ul;
        for (; std::getline(ss, l, ','); ++i) {
            if (i==elements) throw std::format_error("malformed vector: too many elements provided");
            val[i] = reader(l);
        }

        if (i==1) {
            val = V{ val.x };
        }
        else if (i!=elements) throw std::format_error("malformed vector: too few elements provided");

        return val;
    }

    constexpr auto element_names = "x\0y\0z\0w\0";
    static_assert(elements<=4);
    for (auto i=0ul;i<elements;++i)
        val[i] = reader_opt(node[&element_names[2*i]],
                            default_value.value_or(V{})[i]);
    return val;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           vec2_t& out) {
    if (node["name"]==name) {
        out = read_vec_attribute<vec2_t>(node);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<vec2_t>& out) {
    if (node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = read_vec_attribute<vec2_t>(node);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           vec3_t& out) {
    if (node["name"]==name) {
        out = read_vec_attribute<vec3_t>(node);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<vec3_t>& out) {
    if (node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = read_vec_attribute<vec3_t>(node);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           vec4_t& out) {
    if (node["name"]==name) {
        out = read_vec_attribute<vec4_t>(node);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<vec4_t>& out) {
    if (node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = read_vec_attribute<vec4_t>(node);
        return true;
    }
    return false;
}


inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           pqvec2_t& out) {
    if (node["name"]==name) {
        out = read_vec_attribute<pqvec2_t>(node);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<pqvec2_t>& out) {
    if (node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = read_vec_attribute<pqvec2_t>(node);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           pqvec3_t& out) {
    if (node["name"]==name) {
        out = read_vec_attribute<pqvec3_t>(node);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<pqvec3_t>& out) {
    if (node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = read_vec_attribute<pqvec3_t>(node);
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           pqvec4_t& out) {
    if (node["name"]==name) {
        out = read_vec_attribute<pqvec4_t>(node);
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<pqvec4_t>& out) {
    if (node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = read_vec_attribute<pqvec4_t>(node);
        return true;
    }
    return false;
}


/* node data parsing: system paths */

inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::filesystem::path& out) {
    if (node.name()=="path" &&
        node["name"]==name) {
        out = node["value"];
        return true;
    }
    return false;
}
inline bool read_attribute(const node_t& node, 
                           const std::string& name, 
                           std::optional<std::filesystem::path>& out) {
    if (node.name()=="path" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = node["value"];
        return true;
    }
    return false;
}

inline bool read_attribute(const node_t& node, 
                           std::optional<std::filesystem::path>& out) {
    if (node.name()=="path") {
        if (out)
            throw std::format_error("Path already provided");
        out = node["value"];
        return true;
    }
    return false;
}


/* node data parsing: ranges */

template <typename T>
inline void load_range_from_attribute(
        const node_t& node,
        const std::string& value_attribute,
        std::optional<range_t<T>>& out) {
    if (out)
        throw std::format_error("Range already provided");
    const std::string& range_str = node[value_attribute];

    range_t<> r;
    parse_range(range_str, r);
    out = r;
}

template <typename T>
inline bool read_attribute(const node_t& node,
                           std::optional<range_t<T>>& out) {
    if (node.name()=="range") {
        load_range_from_attribute(node, "value", out);
        return true;
    }
    return false;
}
template <typename T>
inline bool read_attribute(const node_t& node,
                           const std::string& name,
                           std::optional<range_t<T>>& out) {
    if (node.name()=="range" &&
        node["name"]==name) {
        load_range_from_attribute(node, "value", out);
        return true;
    }
    return false;
}


/* node data parsing: functions */

inline void load_function_from_attribute(
        const node_t& node,
        const std::string& value_attribute,
        std::optional<compiled_math_expression_t>& out,
        const std::vector<std::string>& vars) {
    if (out)
        throw std::format_error("Function already provided");
    {
        std::string func = node[value_attribute];
        // compile expression
        out.emplace(std::move(func),vars);
    }

    // test
    std::vector<f_t> vs;
    vs.resize(out->get_variable_count(), 0);
    [[maybe_unused]] const auto testresult = out->eval(vs);
}

inline bool load_function(const node_t& node,
                          std::optional<compiled_math_expression_t>& out,
                          const std::vector<std::string>& vars) {
    if (node.name()=="function") {
        load_function_from_attribute(node, "value", out, vars);
        return true;
    }
    return false;
}
inline bool load_function(const node_t& node,
                          const std::string& name,
                          std::optional<compiled_math_expression_t>& out,
                          const std::vector<std::string>& vars) {
    if (node.name()=="function" &&
        node["name"]==name) {
        load_function_from_attribute(node, "value", out, vars);
        return true;
    }
    return false;
}


/* node data parsing: Transforms */

inline bool load_transform(const node_t& node,
                           transform_f_t& out,
                           loader_t* loader) {
    if (node.name()=="transform") {
        out = ::wt::load_transform_sfp(node, loader);
        return true;
    }
    else if (node.name() == "ref") {
        const auto id = node["id"];
        const auto rnode = loader->get_node_with_id(id);
        if (!rnode)
            return false;

        // disallow recursive references
        if ((*rnode)->name() == "ref")
            return false;

        return load_transform(**rnode, out, loader);
    }

    return false;
}
inline bool load_transform(const node_t& node,
                           transform_d_t& out,
                           loader_t* loader) {
    if (node.name()=="transform") {
        out = ::wt::load_transform_dfp(node, loader);
        return true;
    }
    else if (node.name() == "ref") {
        const auto id = node["id"];
        const auto rnode = loader->get_node_with_id(id);
        if (!rnode)
            return false;

        // disallow recursive references
        if ((*rnode)->name() == "ref")
            return false;

        return load_transform(**rnode, out, loader);
    }

    return false;
}
inline bool load_transform(const node_t& node,
                           std::optional<transform_f_t>& out,
                           loader_t* loader) {
    if (node.name()=="transform") {
        if (out)
            throw std::format_error("Node \"" + node.name() + "\" already provided");
        out = ::wt::load_transform_sfp(node, loader);
        return true;
    }
    else if (node.name() == "ref") {
        const auto id = node["id"];
        const auto rnode = loader->get_node_with_id(id);
        if (!rnode)
            return false;

        // disallow recursive references
        if ((*rnode)->name() == "ref")
            return false;

        return load_transform(**rnode, out, loader);
    }

    return false;
}
inline bool load_transform(const node_t& node,
                           std::optional<transform_d_t>& out,
                           loader_t* loader) {
    if (node.name()=="transform") {
        if (out)
            throw std::format_error("Node \"" + node.name() + "\" already provided");
        out = ::wt::load_transform_dfp(node, loader);
        return true;
    }
    else if (node.name() == "ref") {
        const auto id = node["id"];
        const auto rnode = loader->get_node_with_id(id);
        if (!rnode)
            return false;

        // disallow recursive references
        if ((*rnode)->name() == "ref")
            return false;

        return load_transform(**rnode, out, loader);
    }

    return false;
}

inline bool load_transform(const node_t& node, 
                           const std::string& name, 
                           transform_f_t& out,
                           loader_t* loader) {
    if (node.name()=="transform" &&
        node["name"]==name) {
        out = ::wt::load_transform_sfp(node, loader);
        return true;
    }
    else if (node.name() == "ref" ||
             node["name"]==name) {
        const auto id = node["id"];
        const auto rnode = loader->get_node_with_id(id);
        if (!rnode)
            return false;

        // disallow recursive references
        if ((*rnode)->name() == "ref")
            return false;

        return load_transform(**rnode, out, loader);
    }

    return false;
}
inline bool load_transform(const node_t& node, 
                           const std::string& name, 
                           transform_d_t& out,
                           loader_t* loader) {
    if (node.name()=="transform" &&
        node["name"]==name) {
        out = ::wt::load_transform_dfp(node, loader);
        return true;
    }
    else if (node.name() == "ref" ||
             node["name"]==name) {
        const auto id = node["id"];
        const auto rnode = loader->get_node_with_id(id);
        if (!rnode)
            return false;

        // disallow recursive references
        if ((*rnode)->name() == "ref")
            return false;

        return load_transform(**rnode, out, loader);
    }

    return false;
}
inline bool load_transform(const node_t& node, 
                           const std::string& name, 
                           std::optional<transform_f_t>& out,
                           loader_t* loader) {
    if (node.name()=="transform" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = ::wt::load_transform_sfp(node, loader);
        return true;
    }
    else if (node.name() == "ref" ||
             node["name"]==name) {
        const auto id = node["id"];
        const auto rnode = loader->get_node_with_id(id);
        if (!rnode)
            return false;

        // disallow recursive references
        if ((*rnode)->name() == "ref")
            return false;

        return load_transform(**rnode, out, loader);
    }

    return false;
}
inline bool load_transform(const node_t& node, 
                           const std::string& name, 
                           std::optional<transform_d_t>& out,
                           loader_t* loader) {
    if (node.name()=="transform" &&
        node["name"]==name) {
        if (out)
            throw std::format_error("Node \"" + name + "\" already provided");
        out = ::wt::load_transform_dfp(node, loader);
        return true;
    }
    else if (node.name() == "ref" ||
             node["name"]==name) {
        const auto id = node["id"];
        const auto rnode = loader->get_node_with_id(id);
        if (!rnode)
            return false;

        // disallow recursive references
        if ((*rnode)->name() == "ref")
            return false;

        return load_transform(**rnode, out, loader);
    }

    return false;
}


/* node data parsing: spectra and textures */

// Loads a scene element node via its name attribute
template <std::derived_from<scene::scene_element_t> SceneElement>
inline bool load_scene_element(const node_t& node, 
                               const std::string& name, 
                               std::shared_ptr<SceneElement>& out,
                               loader_t *loader,
                               const wt_context_t& ctx) {
    const auto cls = SceneElement::scene_element_class();
    if (node["name"] == name &&
        node.name() == cls) {
        if (auto p = std::dynamic_pointer_cast<SceneElement>(
                            std::shared_ptr(SceneElement::load("", loader, node, ctx)));
            p) {
            if (out)
                throw std::format_error("Node \"" + name + "\" already provided");
            out = std::move(p);
            return true;
        }
        else
            throw std::format_error("Failed loading node \"" + name + "\"");
    }
    else if (node.name() == "ref") {
        const auto id = node["id"];
        if (auto p = loader->template get_scene_element<SceneElement>(id); p) {
            if (out)
                throw std::format_error("Node \"" + name + "\" already provided");
            out = std::move(p);
            return true;
        }
    }

    return false;
}

// Loads a scene element node via its class
template <std::derived_from<scene::scene_element_t> SceneElement>
inline bool load_scene_element(const node_t& node, 
                               std::shared_ptr<SceneElement>& out,
                               loader_t *loader,
                               const wt_context_t& ctx) {
    const auto cls = SceneElement::scene_element_class();
    if (node.name() == cls) {
        if (auto p = std::dynamic_pointer_cast<SceneElement>(
                            std::shared_ptr(SceneElement::load("", loader, node, ctx)));
            p) {
            if (out)
                throw std::format_error("Node of class '" + cls + "' already provided");
            out = std::move(p);
            return true;
        }
        else
            throw std::format_error("Failed loading node of class '" + cls + "'");
    }
    else if (node.name() == "ref") {
        const auto id = node["id"];
        if (auto p = loader->template get_scene_element<SceneElement>(id); p) {
            if (out)
                throw std::format_error("Node of class '" + cls + "' already provided");
            out = std::move(p);
            return true;
        }
    }

    return false;
}

// Loads a scene element node via its name attribute
inline bool load_scene_element(const node_t& node, 
                               const std::string& name, 
                               std::shared_ptr<spectrum::spectrum_real_t>& out,
                               loader_t *loader,
                               const wt_context_t& ctx) {
    using SceneElement = spectrum::spectrum_t;
    const auto cls = SceneElement::scene_element_class();
    if (node["name"] == name &&
        node.name() == cls) {
        if (auto p = std::dynamic_pointer_cast<SceneElement>(
                            std::shared_ptr(SceneElement::load("", loader, node, ctx)));
            p) {
            if (out)
                throw std::format_error("Node \"" + name + "\" already provided");

            auto rp = std::dynamic_pointer_cast<spectrum::spectrum_real_t>(std::move(p));
            if (!rp)
                throw std::format_error("Expected a real spectrum");

            out = std::move(rp);
            return true;
        }
        else
            throw std::format_error("Failed loading node \"" + name + "\"");
    }
    else if (node.name() == "ref") {
        const auto id = node["id"];
        if (auto p = loader->template get_scene_element<SceneElement>(id); p) {
            if (out)
                throw std::format_error("Node \"" + name + "\" already provided");

            auto rp = std::dynamic_pointer_cast<spectrum::spectrum_real_t>(std::move(p));
            if (!rp)
                throw std::format_error("Expected a real spectrum");

            out = std::move(rp);
            return true;
        }
    }

    return false;
}

// Loads a texture, or a spectrum and converts it to a constant texture
inline bool load_texture_element(const node_t& node, 
                                 const std::string& name, 
                                 std::shared_ptr<texture::texture_t>& out,
                                 loader_t *loader,
                                 const wt_context_t& ctx) {
    if (load_scene_element(node, name, out, loader, ctx))
        return true;
    
    std::shared_ptr<spectrum::spectrum_t> spectrum;
    if (load_scene_element(node, name, spectrum, loader, ctx)) {
        auto rspectrum = std::dynamic_pointer_cast<spectrum::spectrum_real_t>(std::move(spectrum));
        if (!rspectrum)
            return false;
        auto id = rspectrum->get_id()+"_texture";
        out = std::make_shared<texture::constant_t>(std::move(id), std::move(rspectrum));
        return true;
    }

    return false;
}

// Loads a texture, or a spectrum and converts it to a constant texture
inline bool load_texture_element(const node_t& node, 
                                 std::shared_ptr<texture::texture_t>& out,
                                 loader_t *loader,
                                 const wt_context_t& ctx) {
    if (load_scene_element(node, out, loader, ctx))
        return true;
    
    std::shared_ptr<spectrum::spectrum_t> spectrum;
    if (load_scene_element(node, spectrum, loader, ctx)) {
        auto rspectrum = std::dynamic_pointer_cast<spectrum::spectrum_real_t>(std::move(spectrum));
        if (!rspectrum)
            return false;
        auto id = rspectrum->get_id()+"_texture";
        out = std::make_shared<texture::constant_t>(std::move(id), std::move(rspectrum));
        return true;
    }

    return false;
}

// Loads a texture, or a spectrum and converts it to a constant texture
inline bool load_texture_element(const node_t& node, 
                                 const std::string& name, 
                                 std::shared_ptr<texture::complex_t>& out,
                                 loader_t *loader,
                                 const wt_context_t& ctx) {
    if (load_scene_element(node, name, out, loader, ctx))
        return true;
    
    std::shared_ptr<spectrum::spectrum_t> spectrum;
    if (load_scene_element(node, name, spectrum, loader, ctx)) {
        auto id = spectrum->get_id()+"_texture";
        out = std::make_shared<texture::complex_constant_t>(std::move(id), std::move(spectrum));
        return true;
    }

    return false;
}

// Loads a texture, or a spectrum and converts it to a constant texture
inline bool load_texture_element(const node_t& node, 
                                 std::shared_ptr<texture::complex_t>& out,
                                 loader_t *loader,
                                 const wt_context_t& ctx) {
    if (load_scene_element(node, out, loader, ctx))
        return true;
    
    std::shared_ptr<spectrum::spectrum_t> spectrum;
    if (load_scene_element(node, spectrum, loader, ctx)) {
        auto id = spectrum->get_id()+"_texture";
        out = std::make_shared<texture::complex_constant_t>(std::move(id), std::move(spectrum));
        return true;
    }

    return false;
}

}
