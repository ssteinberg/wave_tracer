/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/interaction/intersection.hpp>

#include <wt/beam/gaussian_wavefront.hpp>

#include <wt/scene/shape.hpp>
#include <wt/bsdf/bsdf.hpp>
#include <wt/ads/intersection_record.hpp>
#include <wt/mesh/surface_differentials.hpp>

#include <wt/math/common.hpp>
#include <wt/math/quantity/defs.hpp>
#include <wt/math/linalg.hpp>
#include <wt/math/barycentric.hpp>
#include <wt/math/eft/eft.hpp>

#include <wt/util/assert.hpp>

using namespace wt;

const mesh::surface_differentials_t& intersection_surface_t::tangent_frame() const noexcept {
    return shape->get_mesh().tangent_frame(mesh_tri_idx);
}

intersection_surface_t::intersection_surface_t(
        const shape_t* shape,
        const dir3_t& geo_n,
        const tidx_t mesh_tri_idx,
        const barycentric_t::triangle_point_t& bary_point,
        const mesh::surface_differentials_t& tf,
        const pqvec3_t& beam_intersection_centre) noexcept
    : wp(beam_intersection_centre),
      uv(bary_point.uv),
      bary(bary_point.bary),
      mesh_tri_idx(mesh_tri_idx),
      shape(shape),
      geo(frame_t::build_shading_frame(geo_n, tf.dpdu)),
      shading(shape->get_bsdf().shading_frame(
                texture::texture_query_t{
                    .uv = uv,
                    .k = f_t(0)/u::mm,      // assume shading frame construction does not depend on wavenumber
                    .pdvs = {},             // assume pdvs are not needed
                },
                tf,
                bary_point.n))
{}

intersection_surface_t::intersection_surface_t(
        const shape_t* shape,
        const dir3_t& geo_n,
        const intersection_surface_t::tidx_t mesh_tri_idx,
        const barycentric_t& bary,
        const pqvec3_t& beam_intersection_centre) noexcept
    : intersection_surface_t(shape,
                             geo_n,
                             mesh_tri_idx,
                             bary(shape->get_mesh().triangle(mesh_tri_idx)),
                             shape->get_mesh().tangent_frame(mesh_tri_idx),
                             beam_intersection_centre)
{}

intersection_surface_t::intersection_surface_t(const shape_t* shape,
                                               const intersection_surface_t::tidx_t mesh_tri_idx,
                                               const barycentric_t& bary) noexcept
    : intersection_surface_t(shape,
                             shape->get_mesh().triangle_face_normal(mesh_tri_idx),
                             mesh_tri_idx, 
                             bary(shape->get_mesh().triangle(mesh_tri_idx)),
                             shape->get_mesh().tangent_frame(mesh_tri_idx))
{}

texture::texture_query_t intersection_surface_t::texture_query(const wavenumber_t& k) const noexcept {
    texture::texture_query_t ret = {
        .uv = uv,
        .k = k,
    };
    if (shape->get_bsdf().needs_interaction_footprint())
        ret.pdvs = pdvs_at_intersection();
    return ret;
}


intersection_uv_pdvs_t intersection_surface_t::pdvs_at_intersection() const noexcept {
    constexpr auto scale = f_t(.5) / beam::gaussian_wavefront_t::beam_cross_section_envelope;

    // compute pdvs of uv w.r.t. tb position in local shading frame
    qvec2<length_density_t> dudtb, dvdtb;
    {
        const auto& tri = shape->get_mesh().triangle(mesh_tri_idx);
        if (!tri.uv)
            return {};

        const auto& uv0 = (*tri.uv)[0];
        const auto& uv1 = (*tri.uv)[1];
        const auto& uv2 = (*tri.uv)[2];
        const auto tb0 = pqvec2_t{ shading.to_local(tri.p[0]) };
        const auto tb1 = pqvec2_t{ shading.to_local(tri.p[1]) };
        const auto tb2 = pqvec2_t{ shading.to_local(tri.p[2]) };

        const auto a = tb1.x-tb0.x;
        const auto b = tb1.y-tb0.y;
        const auto c = tb2.x-tb0.x;
        const auto d = tb2.y-tb0.y;
        const auto tbdet = m::eft::diff_prod(a,d, b,c);
        assert(tbdet!=zero);
        if (tbdet==zero)
            return {};

        const auto rcp = f_t(1) / tbdet;
        dudtb = qvec2<length_density_t>{
            m::eft::diff_prod((uv1.x-uv0.x),d, (uv2.x-uv0.x),b) * rcp,
            m::eft::diff_prod((uv2.x-uv0.x),a, (uv1.x-uv0.x),c) * rcp
        };
        dvdtb = qvec2<length_density_t>{
            m::eft::diff_prod((uv1.y-uv0.y),d, (uv2.y-uv0.y),b) * rcp,
            m::eft::diff_prod((uv2.y-uv0.y),a, (uv1.y-uv0.y),c) * rcp
        };
    }

    // ... then, compute pdvs of uv w.r.t. cone footprint ellipse axes a and b

    // transform a and b from geo frame to shading frame
    const auto& a = pqvec2_t{ shading.to_local(geo.to_world(footprint.a())) };
    const auto& b = pqvec2_t{ shading.to_local(geo.to_world(footprint.b())) };

    // basis change pdvs to a,b frame
    const auto dudab = vec2_t{ m::eft::dot(a, dudtb), m::eft::dot(b, dudtb) };
    const auto dvdab = vec2_t{ m::eft::dot(a, dvdtb), m::eft::dot(b, dvdtb) };
    auto ret = intersection_uv_pdvs_t{
        .duda = dudab.x * scale,
        .dudb = dudab.y * scale,
        .dvda = dvdab.x * scale,
        .dvdb = dvdab.y * scale,
    };
    assert(m::isfinite(ret.duda) && m::isfinite(ret.dvda) && m::isfinite(ret.dvdb) && m::isfinite(ret.dvdb));

    return ret;
}


inline pqvec3_t compute_intersection_triangle_fp_errors(
        const pqvec3_t& a, const pqvec3_t& b, const pqvec3_t& c,
        const pqvec3_t& ro) noexcept {
    // Adapted from https://developer.nvidia.com/blog/solving-self-intersection-artifacts-in-directx-raytracing/
    // increased constants
    // TODO: to better
    const f_t c0 = f_t(3e-6);
    const f_t c1 = f_t(5e-6);
    const f_t c2 = f_t(3e-6);

    // compute twice the maximum extent of the triangle 
    const auto v0 = m::abs(a);
    const auto e1 = m::abs(b-a);
    const auto e2 = m::abs(c-a);
    const auto extents = e1 + e2 + m::abs(e1 - e2);
    const auto extent = m::max_element(extents); 

    const auto obj_err = (c0+c2)*v0 + pqvec3_t{ c1*extent };
    const auto wrld_err = (c1+c2)*m::abs(ro);

    return obj_err + wrld_err;
}

pqvec3_t intersection_surface_t::offseted_ray_origin(const ray_t& ray) const noexcept {
    if (!shape)
        return ray.o;

    const auto& tri = shape->get_mesh().triangle(mesh_tri_idx);
    
    const auto intersection_fp_error = compute_intersection_triangle_fp_errors(
            tri.p[0], tri.p[1], tri.p[2], ray.o);
    const auto offset_dist = m::dot(intersection_fp_error, m::abs(vec3_t{ ng() }));
    const auto offset = offset_dist * ng();

    // offset along face normal in direction of ray travel
    return ray.o + (m::dot(ray.d,offset)>=zero ? offset : -offset);
}

pqvec3_t intersection_edge_t::offseted_ray_origin(const ray_t& ray) const noexcept {
    const auto* t1 = edge->tri1;
    const auto* t2 = edge->tri2;

    // offset away from the wedge
    dir3_t dir = { 0,0,1 };
    if (!t2) dir = -edge->t1;
    else {
        const auto v = vec3_t{ edge->t1 } + vec3_t{ edge->t2 };
        dir = m::length2(v)>f_t(1e-14) ? -m::normalize(v) : -edge->t2;
    }

    const auto t1_fp_err = compute_intersection_triangle_fp_errors(
        t1->a, t1->b, t1->c, ray.o);
    auto d = m::dot(t1_fp_err, m::abs(vec3_t{ edge->t1 }));

    if (t2) {
        const auto t2_fp_err = compute_intersection_triangle_fp_errors(
            t2->a, t2->b, t2->c, ray.o);
        const auto d2 = m::dot(t2_fp_err, m::abs(vec3_t{ edge->t2 }));
        d = m::max(d,d2);
    }

    return ray.o + d*dir;
}
