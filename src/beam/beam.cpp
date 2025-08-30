/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <cassert>

#include <wt/beam/beam_generic.hpp>

#include <wt/math/common.hpp>
#include <wt/math/linalg.hpp>

using namespace wt;
using namespace wt::beam;


intersection_footprint_t beam_generic_t::surface_footprint_ellipsoid(
        const intersection_surface_t& surface, 
        const length_t beam_z_dist) const noexcept {
    const auto ls = footprint(beam_z_dist);
    if (ls.x == 0*u::m || ls.y == 0*u::m || ls.z == 0*u::m) {
        // degenerate footprint
        assert(ls == ls.zero());
        return {};
    }

    // transform to interaction point centre
    const auto p = surface.wp - (envelope.o() + beam_z_dist*envelope.d());
    const auto plane_d = m::eft::dot(p,surface.ng());
    const auto& frame = surface.geo;
    const auto& n = frame.n;

    // convert footprint ellipsoid into a (unit) sphere and intersect there
    const auto& t = ls;
    const auto nn = m::normalize(t*vec3_t{ n });
    const auto dd = (f_t)m::eft::dot(plane_d*n / t, nn);

    if (m::abs(dd)>1) {
        // point outside the ls footprint?
        return {};
    }

    const auto fc = frame_t::build_orthogonal_frame(nn);  // sphere's frame

    // ... then transform back to ellipsoid
    const auto r = m::sqrt(m::max<f_t>(0,1-m::sqr(dd)));
    const auto t1 = r * t * vec3_t{ fc.t };
    const auto t2 = r * t * vec3_t{ fc.b };

    // ... and SVD in-order to reconstruct intersection ellipse
    const auto A = mat2_t{ 
        frame.to_local(u::to_m(pqvec2_t{ t1 })), 
        u::to_m(frame.to_local(pqvec2_t{ t2 }))
    };
    const auto svd = SVD(A);
    auto X = dir2_t{ svd.Ucos, -svd.Usin };
    auto lX = m::abs(svd.sigma1) * (1*u::m);
    auto lY = m::abs(svd.sigma2) * (1*u::m);
    if (lX<lY) {
        std::swap(lX,lY);
        X = dir2_t{ svd.Usin, svd.Ucos };
    }

    assert(lX>=zero && lY>=zero && m::isfinite(lX) &&m::isfinite(lY));

    return {
        .x = X,
        .la = lX,
        .lb = lY,
    };
}
