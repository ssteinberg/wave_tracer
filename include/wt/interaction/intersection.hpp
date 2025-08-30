/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/ads/common.hpp>

#include <wt/mesh/mesh.hpp>
#include <wt/mesh/surface_differentials.hpp>

#include <wt/texture/texture.hpp>

#include <wt/math/common.hpp>
#include <wt/math/frame.hpp>
#include <wt/math/barycentric.hpp>
#include <wt/math/intersect/intersect_defs.hpp>

#include "common.hpp"

namespace wt {

class shape_t;


/**
 * @brief Describes a beam-surface intersection geometry.
 */
struct intersection_surface_t {
    using tidx_t = mesh::mesh_t::tidx_t;

    /** @brief Centre world position point of beam-surface intersection.
     */
    pqvec3_t wp;

    /** @brief Linearly-interpolated vertex UV coordinates at interaction centre.
     */
    vec2_t uv;

    /** @brief The barycentric coordinates.
     */
    barycentric_t bary;

    /** @brief Footprint of beam intersection (spanned in geometric ``geo`` tangent frame, centred around ``wp``).
     */
    intersection_footprint_t footprint;
    
    tidx_t mesh_tri_idx;
    const shape_t* shape = nullptr;

    /** @brief Geometric frame.
     */
    frame_t geo;
    /** @brief Shading frame.
     *         Shading frame is defined by the linearly-interpolated vertices' shading normals, and potentially further perturbed by the shape's BSDF (e.g., if the BSDF does normal mapping).
     */
    frame_t shading;


private:
    intersection_surface_t(const shape_t* shape,
                           const dir3_t& geo_n,
                           const tidx_t mesh_tri_idx,
                           const barycentric_t::triangle_point_t& bary_point,
                           const mesh::surface_differentials_t& tf,
                           const pqvec3_t& beam_intersection_centre) noexcept;
    inline intersection_surface_t(const shape_t* shape,
                                  const dir3_t& geo_n,
                                  const tidx_t mesh_tri_idx,
                                  const barycentric_t::triangle_point_t& bary_point,
                                  const mesh::surface_differentials_t& tf) noexcept
        : intersection_surface_t(shape, geo_n, mesh_tri_idx, bary_point, tf, bary_point.p)
    {}

public:
    intersection_surface_t(const shape_t* shape,
                           const dir3_t& geo_n,
                           const tidx_t mesh_tri_idx,
                           const barycentric_t& bary,
                           const pqvec3_t& beam_intersection_centre) noexcept;

    intersection_surface_t(const shape_t* shape,
                           const tidx_t mesh_tri_idx,
                           const barycentric_t& bary) noexcept;

    inline intersection_surface_t(const shape_t* shape,
                                  const ads::tri_t& ads_tri,
                                  const ray_t& ray,
                                  const intersect::intersect_ray_tri_ret_t& ray_intersection_record) noexcept
        : intersection_surface_t(shape, ads_tri.n,
                                 ads_tri.shape_tri_idx, 
                                 ray_intersection_record.bary, 
                                 ray.propagate(ray_intersection_record.dist))
    {}

    // dummy surface (no associated shape -- like a virtual coverage sensor) intersection
    inline intersection_surface_t(const dir3_t& geo_n,
                                  const pqvec3_t& beam_intersection_centre) noexcept
        : wp(beam_intersection_centre),
          geo(frame_t::build_orthogonal_frame(geo_n)),
          shading(frame_t::build_orthogonal_frame(geo_n))
    {}

    intersection_surface_t(const intersection_surface_t&) noexcept = default;


    [[nodiscard]] inline const auto& ng() const noexcept { return geo.n; }
    [[nodiscard]] inline const auto& ns() const noexcept { return shading.n; }

    [[nodiscard]] const mesh::surface_differentials_t& tangent_frame() const noexcept;

    /**
     * @brief Returns the s-polarization direction in world coordinates (normal to incidence plane).
     * @param w incident or outgoing direction
     */
    [[nodiscard]] inline auto s_direction(const dir3_t& w) const noexcept {
        const auto crs = m::cross(w, shading.n);
        const auto l2  = m::length2(crs);
        const auto ret = l2<f_t(1e-14) ? 
            shading.t : 
            dir3_t{ crs/m::sqrt(l2) };

        // flip direction when w is pointing inwards, this ensures that the sp frame is identical for w that points in as well as out of the surface.
        return m::dot(w,shading.n)<0 ? -ret : ret;
    }

    /**
     * @brief Constructs the sp frame, where ``t`` is the s-polarization direction (normal to incidence plane) and ``b`` is the p-polarization direction (aligns with incidence plane).
     * @param w incident or outgoing direction
     */
    [[nodiscard]] inline auto sp_frame(const dir3_t& w) const noexcept {
        const auto s = s_direction(w);
        const auto p = dir3_t{ m::cross(s, w) };
        return frame_t{
            .t = s,
            .b = m::dot(w,shading.n)<0 ? -p : p,
            .n = w,
        };
    }

    /**
     * @brief Constructs a texture query structure.
     *        When the shape BSDF sets ``needs_interaction_footprint()`` to TRUE, this also populates the appropriate partial derivatives in the texture query by calling pdvs_at_intersection().
     */
    [[nodiscard]] texture::texture_query_t texture_query(const wavenumber_t& k) const noexcept;

    /**
    * @brief Computes the partial derivatives w.r.t. to beam footprint.
    */
    [[nodiscard]] intersection_uv_pdvs_t pdvs_at_intersection() const noexcept;

    /**
     * @brief Computes an offseted origin that avoids self-intersection
     */
    [[nodiscard]] pqvec3_t offseted_ray_origin(const ray_t& ray) const noexcept;
};


/**
 * @brief Describes a beam-edge intersection geometry.
 */
struct intersection_edge_t {
    // intersected edge
    const ads::edge_t* edge;

    // point of edge intersection
    pqvec3_t wp;

    inline intersection_edge_t(const ads::edge_t& edge, const pqvec3_t& wp) noexcept
        : edge(&edge),
          wp(wp)
    {}

    intersection_edge_t(const intersection_edge_t&) noexcept = default;

    /**
     * @brief Computes an offseted origin that avoids self-intersection
     */
    [[nodiscard]] pqvec3_t offseted_ray_origin(const ray_t& ray) const noexcept;

    /**
     * @brief Constructs the sh frame ("soft" and "hard" diffraction frames of UTD), where ``t`` is the "soft"-polarization direction (in the plane that contains the edge and the ray direction) and ``b`` is the "hard"-polarization direction (orthogonal to the plane that contains the edge and the ray direction).
     * @param w incident or outgoing direction
     */
    [[nodiscard]] inline auto sh_frame(const dir3_t& w) const noexcept {
        const auto& e = edge->e;
        const auto& phi  = m::normalize(m::cross(w,e));
        const auto& beta = dir3_t{ m::cross(phi,w) };
        return frame_t{
            .t = beta,
            .b = phi,
            .n = w,
        };
    }
};


/**
 * @brief Describes a beam-volume intersection.
 */
struct intersection_volumetric_t {
    // point of intersection
    pqvec3_t wp;

    /**
     * @brief Computes an offseted origin that avoids self-intersection
     */
    [[nodiscard]] pqvec3_t offseted_ray_origin(const ray_t& ray) const noexcept { return ray.o; }
};


template <typename T>
concept Intersection = requires(T param) {
    requires std::is_same_v<T,intersection_surface_t> || 
             std::is_same_v<T,intersection_edge_t> ||
             std::is_same_v<T,intersection_volumetric_t>;
};


}
