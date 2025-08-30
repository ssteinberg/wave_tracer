/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <compare>
#include <wt/math/common.hpp>

namespace wt::ads {

class ads_t;

using idx_t = std::uint32_t;
static constexpr auto invalid_idx = limits<idx_t>::max();

struct tuid_t {
    idx_t uid = invalid_idx;

    inline bool operator!() const noexcept { return uid == invalid_idx; }
    explicit inline operator bool() const noexcept { return !!(*this); }

    inline operator idx_t() const noexcept { return uid; }

    inline std::strong_ordering operator<=>(const tuid_t& o) const noexcept = default;
};


/**
 * @brief Triangle data
 */
struct tri_t {
    pqvec3_t a,b,c;
    dir3_t n;

    // shape
    std::uint32_t shape_idx;
    std::uint32_t shape_tri_idx;

    // edges
    tuid_t edge_ab{}, edge_bc{}, edge_ca{};
};


/**
 * @brief Geometric edge of a triangle or shared by a couple triangles
 */
struct edge_t {
    // vertices
    pqvec3_t a,b;
    dir3_t e;

    // face normals do not have to be equal to the triangle normals: they might be flipped.
    // face normals are given such that they point our of the wedge (alpha<pi)

    // face normals and edge tangent direction (on triangle and pointing into triangle) for triangle 1
    dir3_t n1, t1;
    // face normals and edge tangent direction (on triangle and pointing into triangle) for triangle 2
    dir3_t n2, t2;

    // wedge opening angle
    angle_t alpha;

    // triangles
    const tri_t* tri1;
    const tri_t* tri2;
};

}
