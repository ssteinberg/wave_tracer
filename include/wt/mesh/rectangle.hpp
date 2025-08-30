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

#include <wt/math/common.hpp>
#include <wt/math/transform/transform.hpp>

#include "mesh.hpp"

namespace wt::mesh {

class rectangle_t {
public:
    /**
    * @brief Creates a rectangle with its corners being (-1,-1,0), (1,-1,0), (1,1,0), (-1,1,0), scaled by `scale'
    */
    static mesh_t create(const std::string &shape_id, 
                         const wt_context_t& ctx,
                         const transform_d_t& transform,
                         length_t scale = 1*u::m,
                         std::uint32_t tessellate=1);
    /**
    * @brief Creates a rectangle with its uv=(0,0) corner at point `p', and its u and v edges being `x_edge' and `y_edge' respectively.
    */
    static mesh_t create(const std::string &shape_id,
                         const wt_context_t& ctx,
                         const pqvec3_t& p, const pqvec3_t& x_edge, const pqvec3_t& y_edge,
                         const transform_d_t& transform,
                         std::uint32_t tessellate=1);
};

}
