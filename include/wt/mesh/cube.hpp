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

class cube_t  {
public:
    static mesh_t create(const std::string& shape_id, const wt_context_t& ctx, const transform_d_t& transform, length_t scale = 1*u::m);
};

}
