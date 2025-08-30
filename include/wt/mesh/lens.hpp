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

struct lens_properties_t {
    // minimal thickness
    length_t thickness;

    // face curvatures
    // 0 indicates a planar face; ±1 indicates a half-spherical face.
    // positive and negative values specify convex and concave lens, respectively.
    f_t R1, R2;
};

class lens_t {
public:
    static mesh_t create(const std::string& shape_id, 
                         const wt_context_t& ctx,
                         const pqvec3_t& centre, length_t radius, 
                         const lens_properties_t& lens,
                         const transform_d_t& transform,
                         const std::size_t tessellation=35);
};

}
