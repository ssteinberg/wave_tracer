/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/scene/loader/node.hpp>
#include <wt/math/transform/transform.hpp>

namespace wt {

namespace scene::loader { class loader_t; }

/**
 * @brief Load a transformation in single-precision 32-bit floats.
 */
transform_f_t load_transform_sfp(const scene::loader::node_t& node, scene::loader::loader_t* loader);

/**
 * @brief Load a transformation in double-precision 64-bit floats.
 */
transform_d_t load_transform_dfp(const scene::loader::node_t& node, scene::loader::loader_t* loader);

/**
 * @brief Load a transformation using default `f_t` floating-point type.
 */
inline transform_t load_transform(const scene::loader::node_t& node, scene::loader::loader_t* loader) {
    if constexpr (std::is_same_v<f_t,float>) {
        return static_cast<transform_t>(load_transform_sfp(node, loader));
    }
    else {
        return static_cast<transform_t>(load_transform_dfp(node, loader));
    }
}

}
