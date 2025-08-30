/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/math/common.hpp>
#include <wt/math/frame.hpp>
#include <wt/math/linalg.hpp>
#include <wt/math/intersect/cone.hpp>

#include <wt/math/shapes/elliptic_cone.hpp>

using namespace wt;

elliptic_cone_t elliptic_cone_t::cone_through_ellipse(const pqvec3_t& x, const pqvec3_t& y, const dir3_t& n,
                                                      const ray_t& ray, 
                                                      const f_t tan_alpha, 
                                                      length_t* self_intersection_distance) noexcept {
    if (m::all(m::iszero(x) && m::iszero(y))) {
        // degenerate ray case
        *self_intersection_distance = 0*u::m;
        return elliptic_cone_t{
            ray,
            frame_t::build_orthogonal_frame(ray.d).t,
            0*u::m,
            tan_alpha,
            1, 1,
        };
    }

    assert(m::length2(x)>zero && m::length2(y)>zero);

    // TODO: This is not exact. The ellipse is first projected upon the tangent plane to the cone direction and then
    // an ellipse through that plane is constructed. But the projection is orthographic: the distance to the cone 
    // origin is not initially known.

    // build orthogonal frame around cone direction
    const auto& of = frame_t::build_orthogonal_frame(ray.d);

    const auto xhat = pqvec2_t{ of.to_local(x) };
    const auto yhat = pqvec2_t{ of.to_local(y) };

    // projected ellipse upon orthogonal frame
    const auto svd = SVD(mat2_t{ u::to_m(xhat), u::to_m(yhat) });

    // extract major axes and their lengths
    auto X = dir2_t{ svd.Ucos, -svd.Usin };
    auto lX = m::abs(svd.sigma1) * (1*u::m);
    auto lY = m::abs(svd.sigma2) * (1*u::m);
    if (lX<lY) {
        std::swap(lX,lY);
        X = dir2_t{ svd.Usin, svd.Ucos };
    }

    assert(lY>=0*u::m && lX>=lY && m::isfinite(lX) &&m::isfinite(lY));

    // eccentricity and cone origin distance from centre
    // can produce a degenerate cone
    const auto e  = lY>0*u::m ? (f_t)m::sqrt(lX/lY) : 1;
    const auto wx = of.to_world(X);

    assert_iszero(m::dot(wx,ray.d));
    
    const auto cone = elliptic_cone_t{
        ray,
        wx,
        lX,
        tan_alpha,
        1/e,
        e,
    };

    if (self_intersection_distance) {
        // compute the self intersection distance (from the origin) of the cone with the ellipse surface
        const auto cp_intr = intersect::intersect_cone_plane(cone, n, m::dot(n,ray.o));
        *self_intersection_distance = cp_intr.range.empty() ? 0*u::m : cp_intr.range.max;
        assert(*self_intersection_distance>=0*u::m);
    }

    return cone;
}

elliptic_cone_t elliptic_cone_t::cone_through_ellipsoid(
        const pqvec3_t& axes, 
        const frame_t& axes_frame,
        const ray_t& ray, 
        const f_t tan_alpha) noexcept {
    const auto wolocal = axes_frame.to_local(ray.d);
    const auto& frame = frame_t::build_orthogonal_frame(wolocal);

    // convert footprint ellipsoid into a (unit) sphere and intersect there
    const auto& t = axes;
    const auto nn = m::normalize(t * vec3_t{ wolocal });

    const auto fc = frame_t::build_orthogonal_frame(nn);  // sphere's frame

    // ... then transform back to ellipsoid
    const auto t1 = t * vec3_t{ fc.t };
    const auto t2 = t * vec3_t{ fc.b };
    const auto A = mat2_t{ 
        u::to_m(frame.to_local(pqvec2_t{ t1 })),
        u::to_m(frame.to_local(pqvec2_t{ t2 }))
    };

    // 0 determinant?
    if (A[0][0]*A[1][1]==A[1][0]*A[0][1]) {
        // degenerate ray case
        return elliptic_cone_t{
            ray,
            frame_t::build_orthogonal_frame(ray.d).t,
            0*u::m,
            tan_alpha,
            1, 1,
        };
    }

    // ... and SVD in-order to reconstruct intersection ellipse
    const auto svd = SVD(A);
    auto X = dir2_t{ svd.Ucos, -svd.Usin };
    auto lX = m::abs(svd.sigma1) * (1*u::m);
    auto lY = m::abs(svd.sigma2) * (1*u::m);
    if (lX<lY) {
        std::swap(lX,lY);
        X = dir2_t{ svd.Usin, svd.Ucos };
    }

    assert(lY>=0*u::m && lX>=lY && m::isfinite(lX) &&m::isfinite(lY));

    // eccentricity and cone origin distance from centre
    const auto e  = lY>0*u::m ? (f_t)m::sqrt(lX/lY) : 1;

    const auto X3 = m::normalize(vec3_t{ frame.to_world(X) });
    return elliptic_cone_t{
        ray,
        axes_frame.to_world(X3),
        lX,
        tan_alpha,
        1/e,
        e,
    };
}
