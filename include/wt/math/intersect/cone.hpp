/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <optional>
#include <concepts>
#include <utility>

#include "intersect_defs.hpp"
#include "misc.hpp"
#include "ray.hpp"

#include <wt/math/common.hpp>
#include <wt/math/barycentric.hpp>
#include <wt/math/range.hpp>
#include <wt/math/eft/eft.hpp>
#include <wt/math/simd/wide_vector.hpp>

#include <wt/math/shapes/aabb.hpp>
#include <wt/math/shapes/elliptic_cone.hpp>

namespace wt::intersect {

/**
 * @brief Edge-cone intersection test.
 * @tparam in_local TRUE if p0 and p1 are given in cone's local frame
 * @tparam ray if TRUE treat p0,p1 as a ray from p0 through p1
 * @tparam line if TRUE treat p0,p1 as a line passing through p0 and p1 (ray must be set to TRUE as well)
 * @tparam test_clip_planes if TRUE test intersection with near/far clip planes
 */
template <bool in_local=false, bool ray=false, bool line=false, bool test_clip_planes=true>
inline std::optional<intersect_cone_edge_ret_t> intersect_cone_edge(
        const elliptic_cone_t& cone,
        const pqvec3_t& p0,
        const pqvec3_t& p1,
        const pqrange_t<>& range = pqrange_t<>::positive()) noexcept {
    static_assert(!line || ray, "if line is TRUE, must set ray to TRUE as well");

    pqvec3_t localp0, localp1;
    const auto& frame = cone.frame();
    if constexpr (!in_local) {
        localp0 = frame.to_local(p0-cone.o());
        localp1 = frame.to_local(p1-cone.o());
    } else {
        localp0 = p0; localp1 = p1;
    }

    const bool p0closer = localp1.z>localp0.z;
    if (!p0closer) std::swap(localp0,localp1);

    const auto p = localp0, l = localp1-localp0;
    const auto x0 = cone.x0();
    const auto ta = cone.get_tan_alpha();
    const auto e  = cone.get_e();

    const auto cs = p.z*ta + x0;
    const auto epy = e*p.y;
    const auto ely = e*l.y;
    const auto lzta = l.z*ta;

    const auto c = m::sqr(p.x) + m::eft::diff_prod(epy,epy, cs,cs);
    const auto b = 2 * m::eft::dot(pqvec3_t{ p.x,epy,-lzta }, pqvec3_t{ l.x,ely,cs });
    const auto a = m::sqr(l.x) + m::eft::diff_prod(ely,ely, lzta,lzta);

    const auto D = b*b - 4*a*c;
    if (D<zero)
        return std::nullopt;

    const auto sqrtD = m::sqrt(D);
    auto t1 = (f_t)(b>=zero ? (-b-sqrtD)/(2*a) : (-b+sqrtD)/(2*a));
    auto t2 = (f_t)(-b/a)-t1;

    const auto zapex = cone.get_z_apex();
    if (p.z + t1*l.z<=zapex)
        t1 = limits<f_t>::infinity();
    if (p.z + t2*l.z<zapex)
        t2 = limits<f_t>::infinity();

    if (t2<t1) std::swap(t1,t2);
    auto z1 = t1<limits<f_t>::infinity() ? p.z + t1*l.z : -limits<length_t>::infinity();
    auto z2 = t2<limits<f_t>::infinity() ? p.z + t2*l.z : limits<length_t>::infinity();
    assert(z2>=z1);

    if (z1>range.max || z2<range.min || (!m::isfinite(z1) && !m::isfinite(z2)))
        return std::nullopt;

    if (range.min>zapex && z1<range.min) {
        if constexpr (test_clip_planes)
        if (const auto tmin = intersect_line_plane(p, p+l, pqvec3_t{ 0*u::m,0*u::m,range.min }, dir3_t{ 0,0,1 }); tmin) {
            t1 = *tmin;
            z1 = range.min;
        }
    } else {
        assert(m::isfinite(t1));
    }

    if (z2>range.max) {
        if constexpr (test_clip_planes)
        if (const auto tmax = intersect_line_plane(p, p+l, pqvec3_t{ 0*u::m,0*u::m,range.max }, dir3_t{ 0,0,1 }); tmax) {
            t2 = *tmax;
            z2 = range.max;
        }
    }

    std::optional<pqvec3_t> v1,v2;
    if (line || (t1>=0 && (ray || 1>=t1))) v1 = (p0closer ? p0 : p1) + t1 * (p0closer ? p1-p0 : p0-p1);
    else z1=z2;
    if (line || (t2>=0 && (ray || 1>=t2))) v2 = (p0closer ? p0 : p1) + t2 * (p0closer ? p1-p0 : p0-p1);
    else z2=z1;
    if (!v1 && !v2)
        return std::nullopt;

    intersect_cone_edge_ret_t ret;
    ret.range = { z1,z2 };
    ret.pts = v1&&v2 ? 2 : 1;
    ret.p0 = v1 ? *v1 : *v2;
    if (v1&&v2)
        ret.p1 = *v2;

    return ret;
}
/**
 * @brief Ray-cone intersection test.
 * @tparam in_local TRUE if p0 and p1 are given in cone's local frame
 */
template <bool in_local=false>
inline std::optional<intersect_cone_edge_ret_t> intersect_cone_ray(
        const elliptic_cone_t& cone,
        const ray_t& ray,
        const pqrange_t<>& range = pqrange_t<>::positive()) noexcept {
    return intersect_cone_edge<in_local, true>(cone, ray.o, ray.o + ray.d * (1*u::m), range);
}
/**
 * @brief Line-cone intersection test.
 * @tparam in_local TRUE if p0 and p1 are given in cone's local frame
 */
template <bool in_local=false>
inline std::optional<intersect_cone_edge_ret_t> intersect_cone_line(
        const elliptic_cone_t& cone,
        const pqvec3_t& p0,
        const pqvec3_t& p1,
        const pqrange_t<>& range = pqrange_t<>::positive()) noexcept {
    return intersect_cone_edge<in_local, true, true>(cone, p0, p1, range);
}

/**
 * @brief Ray-AABB intersection test. 
 * @tparam test_points_in_cone if FALSE, assume the caller knows a priori that ALL points are outside the cone.
 */
template <bool in_local=false>
inline bool test_cone_edge(const elliptic_cone_t& cone,
                           const pqvec3_t& p0,
                           const pqvec3_t& p1,
                           const pqrange_t<>& range = pqrange_t<>::positive()) noexcept {
    return !!intersect_cone_edge<in_local>(cone, p0, p1, range);
}

/**
 * @brief Cone-plane intersection test. 
 *        Returns intersection range. If range is empty, no intersection occurs. Returned intersection points are always on the cone boundary.
 * @tparam in_local TRUE if n and d are given in cone's local frame
 */
template <bool in_local=false>
inline intersect_cone_plane_ret_t intersect_cone_plane(
        const elliptic_cone_t& cone,
        dir3_t n, length_t d,
        const pqrange_t<>& range = { 0*u::m, limits<length_t>::infinity() }) noexcept {
    const auto& frame = cone.frame();
    if constexpr (!in_local) {
        d -= m::dot(cone.o(),n);
        n = frame.to_local(n);
    }

    const auto x0 = cone.x0();
    const auto e = cone.get_one_over_e();
    
    // cross sectional cone position where intersection occurs
    const auto v_denom2 = m::sqr(n.x)+m::sqr(e*n.y);
    const auto v = v_denom2>0 ?
        vec2_t{ n.x, e*n.y } / m::sqrt(v_denom2) :
        vec2_t{ 0,0 };
    const auto u =  v * vec2_t{1,e};
    const auto nu = m::dot(n,vec3_t{ u,0 });

    const auto zapex = cone.get_z_apex();
    auto z01 = (d - x0*nu) / (n.z + cone.get_tan_alpha()*nu);
    auto z02 = (d + x0*nu) / (n.z - cone.get_tan_alpha()*nu);

    // classify and order
    const auto has_z01 = z01>=zapex && !m::isnan(z01);
    const auto has_z02 = z02>=zapex && !m::isnan(z02);
    if (!has_z01) z01 = limits<length_t>::infinity();
    if (!has_z02) z02 = limits<length_t>::infinity();
    // positions at intersection candidates
    auto p1 = has_z01 ? pqvec3_t{ (z01*cone.get_tan_alpha() + x0) *   u , z01 } : pqvec3_t::infinity();
    auto p2 = has_z02 ? pqvec3_t{ (z02*cone.get_tan_alpha() + x0) * (-u), z02 } : pqvec3_t::infinity();
    // reorder if needed
    if (z01>z02) {
        std::swap(z01,z02);
        std::swap(p1,p2);
    }

    auto rng = pqrange_t<>{ z01,z02 };
    const bool empty = (!has_z01 && !has_z02) || (rng & range).empty();
    if (empty)
        return { .range=pqrange_t<>::null(), };

    // utility function to compute the point closest to the cone's mean on the plane-plane intersection
    constexpr auto closest_point_plane_plane_intersection = 
        [](const auto z, const auto u, const auto n, const auto d){
        length_t x0,y0;
        if (m::abs(n.y)>m::abs(n.x)) {
            y0 = (d-n.z*z)/n.y;
            x0 = n.x!=zero ? (d-n.z*z-n.y*y0)/n.x : 0*u::m;
        } else {
            x0 = (d-n.z*z)/n.x;
            y0 = n.y!=zero ? (d-n.z*z-n.x*x0)/n.y : 0*u::m;
        }

        return pqvec3_t{ (x0*u.x+y0*u.y)*u, z };
    };

    // clamp and transform to world, if needed
    if (m::isfinite(rng.min)) {
        if (rng.min<range.min) {
            // compute point on plane at z=range.min closest to x=0,y=0
            p1 = closest_point_plane_plane_intersection(range.min,v,n,d);
            rng.min = range.min;
        }
        if constexpr (!in_local)
            p1 = cone.o() + frame.to_world(p1);
    }
    // when one intersection point is behind and one in front of the apex, infinite plane point is contained in the cone.
    const bool has_infinite = has_z01 != has_z02;
    if (m::isfinite(rng.max) || has_infinite) {
        assert(!has_infinite || !m::isfinite(rng.max));
        if (rng.max>range.max) {
            // compute point on plane at z=range.max closest to x=0,y=0
            p2 = closest_point_plane_plane_intersection(range.max,v,n,d);
            rng.max = range.max;
        }
        if constexpr (!in_local)
            p2 = cone.o() + frame.to_world(p2);
    }

    return {
        .range = rng,
        .near  = p1,
        .far   = p2,
    };
}

/**
 * @brief Cone-plane intersection test. 
 * @tparam in_local TRUE if n and d are given in cone's local frame
 */
template <bool in_local=false>
inline bool test_cone_plane(
        const elliptic_cone_t& cone,
        dir3_t n, length_t d,
        const pqrange_t<>& range = { 0*u::m, limits<length_t>::infinity() }) noexcept {
    return !intersect_cone_plane<in_local>(cone, n,d, range).range.empty();
}

namespace detail {

// Check if an AABB may intersect the cone
// conservative acceptance: returns false when the AABB never intersects, 
//                          returns true when it *may* intersect
inline bool fast_check_if_intersection_possible_cone_aabb(
                const elliptic_cone_t& cone,
                const aabb_t& aabb,
                const pqrange_t<>& range = pqrange_t<>::positive()) noexcept {
    const auto& f = cone.frame();

    const auto c = aabb.centre()-cone.o();
    const auto e = aabb.extent()/f_t(2);
    const auto rz = m::abs(m::dot(e,m::abs(vec3_t{ f.n })));
    const auto rx = m::abs(m::dot(e,m::abs(vec3_t{ f.t })));
    const auto ry = m::abs(m::dot(e,m::abs(vec3_t{ f.b })));

    const auto minz = m::dot(c,f.n) - rz;
    const auto maxz = m::dot(c,f.n) + rz;
    const auto axes = cone.axes(maxz);

    const auto x = m::dot(c,f.t);
    const auto y = m::dot(c,f.b);
    return pqrange_t<>{ minz,maxz }.overlaps(range) &&
           pqrange_t<>{ x-rx,x+rx }.overlaps(pqrange_t<>{ -axes.x,+axes.x }) &&
           pqrange_t<>{ y-ry,y+ry }.overlaps(pqrange_t<>{ -axes.y,+axes.y });
}
// Check if the convex hull of a set of points may intersect the cone
// conservative acceptance: returns false when the hull never intersects, 
//                          returns true when it *may* intersect
inline bool fast_check_if_intersection_possible_cone_hull(
                const elliptic_cone_t& cone,
                const pqrange_t<>& range = pqrange_t<>::positive(),
                std::convertible_to<pqvec3_t> auto&& ...pts) noexcept {
    // AABB of convex hull
    const auto aabb = aabb_t::from_points(pts...);
    return fast_check_if_intersection_possible_cone_aabb(cone, aabb, range);
}

}

/**
 * @brief Cone-AABB intersection test. 
 *        Fast, conservative approximation
 */
inline bool test_cone_aabb(const elliptic_cone_t& cone,
                           const aabb_t& aabb,
                           const pqrange_t<>& range_input = pqrange_t<>::positive()) noexcept {
    // testing faces is slow. Instead, grow range by the AABB size: conservative approximation that avoids face checks.
    const auto grow = m::abs(m::dot(aabb.extent(),cone.d()));
    const auto range = range_input.grow(grow);

    // Various fast accepts
    if (aabb.contains(cone.o() + range.min * cone.d()) ||
        aabb.contains(cone.o() + range.max * cone.d()))
        return true;
    if (test_ray_aabb(cone.ray(), aabb, range))
        return true;
    if (cone.is_ray())
        return false;

    // Fast reject: intersect per-axis cone AABB
    if (!detail::fast_check_if_intersection_possible_cone_aabb(cone, aabb, range_input))
        return false;

    const auto& o = pqvec3_w8_t{ cone.o() };
    // AABB vertices
    const auto& verts = pqvec3_w8_t{
        m::select<0xaa>(length_w8_t{ aabb.min.x },length_w8_t{ aabb.max.x }),
        m::select<0xcc>(length_w8_t{ aabb.min.y },length_w8_t{ aabb.max.y }),
        m::select<0xf0>(length_w8_t{ aabb.min.z },length_w8_t{ aabb.max.z })
    };
    const auto& local_verts = cone.frame().to_local(verts - o);

    // any vertices contained?
    const auto contains = cone.contains_local(local_verts,range);
    if (m::any(contains)) return true;

    // test all edges
    // TODO: vectorize
    static constexpr vec2u32_t edges[12] = {
        { 0,1 },{ 1,3 },{ 2,3 },{ 0,2 },
        { 4,5 },{ 5,7 },{ 6,7 },{ 4,6 },
        { 0,4 },{ 1,5 },{ 3,7 },{ 2,6 },
    };
    for (const auto& edge : edges) {
        const auto& p0 = verts.read(edge.x);
        const auto& p1 = verts.read(edge.y);
        if (test_cone_edge<true>(cone,p0,p1, range))
            return true;
    }

    return false;
}

/**
 * @brief Cone-AABB intersection test. Returns intersection range. If range is empty, no intersection occurs.
 */
inline pqrange_t<> intersect_cone_aabb(
        const elliptic_cone_t& cone,
        const aabb_t& aabb,
        const pqrange_t<>& range = pqrange_t<>::positive()) noexcept {
    if (cone.is_ray())
        return intersect_ray_aabb(cone.ray(), aabb) & range;

    // Fast reject: intersect per-axis cone AABB
    if (!detail::fast_check_if_intersection_possible_cone_aabb(cone, aabb, range))
        return pqrange_t<>::null();

    const auto& frame = cone.frame();
    const auto& o = cone.o();
    // AABB vertices
    const auto& verts = pqvec3_w8_t{
        m::select<0xaa>(length_w8_t{ aabb.min.x },length_w8_t{ aabb.max.x }),
        m::select<0xcc>(length_w8_t{ aabb.min.y },length_w8_t{ aabb.max.y }),
        m::select<0xf0>(length_w8_t{ aabb.min.z },length_w8_t{ aabb.max.z })
    };
    const auto vs_z = verts.z();
    const auto& local_verts = cone.frame().to_local(verts - pqvec3_w8_t{ o });

    // find points in cone
    const auto contains_mask = cone.contains_local(local_verts,range);
    const auto contains = contains_mask.to_bitmask();

    // min/max z
    const auto pinf = length_w8_t::inf();
    const auto minf = -pinf;
    const auto z_or_pinf = m::selectv(pinf,vs_z, contains_mask);
    const auto z_or_minf = m::selectv(minf,vs_z, contains_mask);

    const auto maxz_h = m::permute2f<0x21>(vs_z, z_or_minf);
    const auto maxz_l = m::permute2f<0x30>(vs_z, z_or_minf);
    const auto minz_h = m::permute2f<0x21>(vs_z, z_or_pinf);
    const auto minz_l = m::permute2f<0x30>(vs_z, z_or_pinf);
    const auto maxz_lh = m::max(maxz_l, maxz_h);
    const auto minz_lh = m::min(minz_l, minz_h);

    // z range of vertices
    auto possible_range = pqrange_t<>{ 
        .min = m::hmin(minz_lh.extract_lower_half()),
        .max = m::hmax(maxz_lh.extract_lower_half())
    };
    // z range of vertices in cone
    auto ret = pqrange_t<>{ 
        .min = m::hmin(minz_lh.extract_upper_half()),
        .max = m::hmax(maxz_lh.extract_upper_half())
    };

    possible_range &= range;
    if (possible_range.empty())
        return pqrange_t<>::null();

    if (aabb.contains(cone.o() + range.min * cone.d()))
        ret |= pqrange_t<>::range(range.min);
    if (range.max<limits<length_t>::infinity() && aabb.contains(cone.o() + range.max * cone.d()))
        ret |= pqrange_t<>::range(range.max);

    // test all edges
    static constexpr vec2u32_t edges[12] = {
        { 0,1 },{ 1,3 },{ 2,3 },{ 0,2 },
        { 4,5 },{ 5,7 },{ 6,7 },{ 4,6 },
        { 0,4 },{ 1,5 },{ 3,7 },{ 2,6 },
    };
    for (const auto& edge : edges) {
        if (contains[edge.x] && contains[edge.y]) continue;

        const auto& p0 = verts.read(edge.x);
        const auto& p1 = verts.read(edge.y);
        const auto ice = intersect_cone_edge<true>(cone,p0,p1, pqrange_t<>::all());
        if (ice)
            ret |= pqrange_t<>{ ice->p0.z, ice->pts==1 ? ice->p0.z: ice->p1.z };
    }

    // test faces
    static constexpr int facesv0[6] = { 0,4,0,2,0,1 };
    for (int i=0;i<6;++i) {
        const auto& a = verts.read(facesv0[i]);
        const auto& fn = aabb_t::face_normal(i);

        const auto n = frame.to_local(fn);
        const auto d = m::dot(a,n);

        const auto icp = intersect_cone_plane<true>(cone, n, d, range);
        if (icp.range.empty()) continue;

        constexpr auto test_point_in_aabb = [](const auto& aabb, const auto& wp, const auto& ln) {  // wp - world point, ln - local normal
            for (int i=0;i<3;++i) {
                if (ln[i]!=0) continue;
                if (aabb.min[i]>wp[i] || aabb.max[i]<wp[i])
                    return false;
            }
            return true;
        };

        // check if points are in AABB
        if (test_point_in_aabb(aabb, frame.to_world(icp.near)+o, fn))
            ret |= pqrange_t<>::range(icp.range.min);
        if (icp.range.length()>zero && test_point_in_aabb(aabb, frame.to_world(icp.far)+o, fn))
            ret |= pqrange_t<>::range(icp.range.max);
    }

    return ret & possible_range;
}

/**
 * @brief Cone-triangle boolean intersection test.
 */
inline bool test_cone_tri(const elliptic_cone_t& cone,
                          const pqvec3_t& a,
                          const pqvec3_t& b,
                          const pqvec3_t& c,
                          const pqrange_t<>& range = pqrange_t<>::positive()) noexcept {
    if (test_ray_tri(cone.ray(), a,b,c, range))
        return true;

    const auto& o = cone.o();
    // transform triangle to local
    const auto tri4 = pqvec3_w4_t{ a,b,c,pqvec3_t::zero() };
    const auto o4   = pqvec3_w4_t{ o };
    const auto& verts = cone.frame().to_local(tri4 - o4);
    const auto& vs_z = verts.z();

    if (m::max(vs_z.reads<0>(), vs_z.reads<1>(), vs_z.reads<2>()) < range.min ||
        m::min(vs_z.reads<0>(), vs_z.reads<1>(), vs_z.reads<2>()) > range.max)
        return false;

    // find points in cone
    const auto contains = cone.contains_local(verts,range).to_bitmask();

    if (contains[0] || contains[1] || contains[2] ||
        test_cone_edge<true>(cone, verts.reads<0>(),  verts.reads<1>(), range) ||
        test_cone_edge<true>(cone, verts.reads<0>(),  verts.reads<2>(), range) ||
        test_cone_edge<true>(cone, verts.reads<1>(),  verts.reads<2>(), range))
        return true;

    if (range.min<=zero)
        return false;
    
    // finally, does the triangle intersect the near/far clip planes?
    // this is faster than testing a cone--plane intersection
    // TODO: vectorize
    pqvec2_t Ns[2], Fs[2];
    int ns=0,fs=0;
    for (int i=0;i<3;++i) {
        const auto j = (i+1)%3;
        const auto vsi = verts.read(i);
        const auto vsj = verts.read(j);

        const auto np = range.min>zero ?
            intersect_edge_plane(vsi,vsj, pqvec3_t{ 0*u::m,0*u::m,range.min }, dir3_t{ 0,0,1 }) : std::nullopt;
        const auto fp = range.max<limits<length_t>::infinity() ? 
            intersect_edge_plane(vsi,vsj, pqvec3_t{ 0*u::m,0*u::m,range.max }, dir3_t{ 0,0,1 }) : std::nullopt;
        if (np && ns<2) Ns[ns++] = pqvec2_t{ *np };
        if (fp && fs<2) Fs[fs++] = pqvec2_t{ *fp };
    }
    if (ns==2) {
        const auto& axes = cone.axes(range.min);
        if (intersect_edge_ellipse(Ns[0], Ns[1], axes.x, axes.y).points>0)
            return true;
    }
    if (fs==2) {
        const auto& axes = cone.axes(range.max);
        if (intersect_edge_ellipse(Fs[0], Fs[1], axes.x, axes.y).points>0)
            return true;
    }

    return false;
}

/**
 * @brief Cone-triangle intersection test. Returns minimal distance to intersection, if any, and intersection point.
 * @param cone_frame vectorized cone frame, can be constructed using ``cone.frame().vectorized()``.
 * @param a triangle vertex
 * @param b triangle vertex
 * @param c triangle vertex
 * @param n triangle normal
 * @param range range over which to look for intersection
 */
inline std::optional<intersect_cone_tri_ret_t> intersect_cone_tri(
        const elliptic_cone_t& cone,
        const pqvec3_t& a,
        const pqvec3_t& b,
        const pqvec3_t& c,
        const dir3_t& n,
        const pqrange_t<>& range = pqrange_t<>::positive()) noexcept {
    if (cone.is_ray()) {
        // degenerate case: ray-triangle intersection
        const auto cr = intersect_ray_tri(cone.ray(), a,b,c, range);
        if (cr) {
            const auto p = cone.ray().propagate(cr->dist);
            return intersect_cone_tri_ret_t{ .dist = cr->dist, .p = p };
        }
        return std::nullopt;
    }

    const auto& frame = cone.frame();
    const auto& o = cone.o();
    // transform triangle to local
    const auto tri4 = pqvec3_w4_t{ a,b,c,pqvec3_t::zero() };
    const auto o4   = pqvec3_w4_t{ o };
    const auto& verts = cone.frame().to_local(tri4 - o4);
    // z distances and vertices in local frame
    const auto& vs_z = verts.z();
    const auto vs0 = verts.reads<0>();
    const auto vs1 = verts.reads<1>();
    const auto vs2 = verts.reads<2>();
    const auto ln = frame.to_local(n);

    // find points in cone
    const auto contains = cone.contains_local(verts,range).to_bitmask();

    // fast reject: all points before near clip or beyond far clip?
    const auto closest_z  = m::min(vs_z.reads<0>(), vs_z.reads<1>(), vs_z.reads<2>());
    const auto farthest_z = m::max(vs_z.reads<0>(), vs_z.reads<1>(), vs_z.reads<2>());
    if (farthest_z < range.min ||
        closest_z  > range.max)
        return std::nullopt;

    // fast accept: closest vertex inside?
    for (int i=0;i<3;++i) {
        if (contains[i] && vs_z.read(i)==closest_z) {
            const auto p = verts.read(i);
            return intersect_cone_tri_ret_t{ .dist = closest_z,
                                             .p    = frame.to_world(p)+o };
        }
    }

    // find closest point on cone-plane intersection conic section
    const auto icp = intersect_cone_plane<true>(cone, ln, m::dot(vs0,ln), range);
    if (!icp.range.empty()) {
        if (util::is_point_in_triangle(icp.near, vs0, vs1, vs2))
            return intersect_cone_tri_ret_t{ .dist = icp.range.min,
                                             .p    = frame.to_world(icp.near)+o };
    }

    // test all edges
    std::optional<pqvec3_t> p;
    for (int i=0;i<3;++i) {
        const auto j = (i+1)%3;
        const auto a = verts.read(i);
        const auto b = verts.read(j);

        if (contains[i] && contains[j]) continue;
        if (a.z>range.max && b.z>range.max) continue;
        if (a.z<range.min && b.z<range.min) continue;

        const auto cp = intersect_cone_edge<true>(cone, a,b, range);
        if (cp && (!p || p->z>cp->p0.z))
            p = cp->p0;
    }

    if (!p) return {};
    return intersect_cone_tri_ret_t{ .dist = p->z,
                                     .p    = frame.to_world(*p)+o };
}

}
