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

class sphere_t {
public:
    static mesh_t create(const std::string& shape_id, 
                         const wt_context_t& ctx,
                         const pqvec3_t& centre, length_t radius, 
                         const transform_d_t& transform,
                         const std::size_t tessellation=20);
};

}
