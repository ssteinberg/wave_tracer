/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <cmath>

#include <wt/math/common.hpp>
#include <wt/math/erf_lut.hpp>
#include <wt/math/distribution/gaussian2d.hpp>
#include <wt/math/util.hpp>
#include <wt/math/intersect/misc.hpp>
#include <wt/math/shapes/aabb.hpp>

#include <wt/math/glm.hpp>
#include <glm/gtx/fast_exponential.hpp>


namespace wt {

namespace detail {

inline f_t erf(const f_t x) noexcept { return m::erf_lut(x); }

inline auto I_gauss_gauss0(f_t a, f_t b, f_t c, f_t d) noexcept {
    const auto n2 = 1 / (a+2*c*c);
    const auto n = m::sqrt(n2);
    return -m::sqrt_pi/2 * n * m::exp(-2*a*m::sqr(d-b*c)*n2) * (
        erf((a*b+2*c*d)*n) -
        erf((a*(1+b)+2*c*(c+d))*n)
    );
}
inline auto I_gauss_gauss1(f_t a, f_t b, f_t c, f_t d) noexcept {
    const auto n2 = 1 / (a+2*c*c);
    const auto n = m::sqrt(n2);
    return -m::sqrt_pi/2 * n * m::exp(-2*a*m::sqr(d-b*c)*n2) * (
        2*erf(a*(d/c-b)*n) +
        erf((a*b+2*c*d)*n) +
        erf((a*(1+b)+2*c*(c+d))*n)
    );
}

inline auto I_gauss0(f_t a, f_t b) noexcept {
    const auto n2 = 1 / a;
    const auto n = m::sqrt(n2);
    return -m::sqrt_pi/2 * n * (
        erf(a*b*n) -
        erf(a*(1+b)*n)
    );
}

inline auto I_gauss1(f_t a, f_t b, f_t c, f_t d) noexcept {
    const auto sa  = m::sqrt(a);
    const auto n   = 1/sa;
    const auto d_c = d/c;
    return -m::sqrt_pi/2 * n * (
        m::sign(b) * erf(sa*m::abs(b)) +
        m::sign(1+b) * erf(sa*m::abs(1+b)) - 
        2*m::sign(b-d_c) * erf(sa*m::abs(b-d_c))
    );
}

inline auto I_gauss_erf(f_t a, f_t b, f_t c, f_t d) noexcept {
    const auto d_c = d/c;
    return 
        ((d!=0&&c!=0) ? m::sign(d) : d==0&&c!=0 ? m::sign(c) : 1) *
        (
            (c!=0 && -d_c>0 && -d_c<1 ? I_gauss1(a,b,c,d) : I_gauss0(a,b)) - 
            2*(
                f_t(0.2936683276537767)*(c!=0 && -d_c>0 && -d_c<1 ? 
                    I_gauss_gauss1(a,b,c*m::sqrt(f_t(0.6517755981618476)),d*m::sqrt(f_t(0.6517755981618476))) :
                    I_gauss_gauss0(a,b,c*m::sqrt(f_t(0.6517755981618476)),d*m::sqrt(f_t(0.6517755981618476)))
                ) + 
                f_t(0.135758042187825)*(c!=0 && -d_c>0 && -d_c<1 ? 
                    I_gauss_gauss1(a,b,c*m::sqrt(f_t(3.250040490513459)),d*m::sqrt(f_t(3.250040490513459))) :
                    I_gauss_gauss0(a,b,c*m::sqrt(f_t(3.250040490513459)),d*m::sqrt(f_t(3.250040490513459)))
                ) + 
                f_t(0.05245255757691102)*(c!=0 && -d_c>0 && -d_c<1 ? 
                    I_gauss_gauss1(a,b,c*m::sqrt(f_t(31.86882707224491)),d*m::sqrt(f_t(31.86882707224491))) :
                    I_gauss_gauss0(a,b,c*m::sqrt(f_t(31.86882707224491)),d*m::sqrt(f_t(31.86882707224491)))
                ) + 
                f_t(0.01673209873360605)*(c!=0 && -d_c>0 && -d_c<1 ? 
                    I_gauss_gauss1(a,b,c*m::sqrt(f_t(778.6613983601425)),d*m::sqrt(f_t(778.6613983601425))) :
                    I_gauss_gauss0(a,b,c*m::sqrt(f_t(778.6613983601425)),d*m::sqrt(f_t(778.6613983601425)))
                )
            )
        );
}

}

f_t gaussian2d_t::integrate_triangle(
        vec2_t a,
        vec2_t b,
        vec2_t c) const noexcept {
    if (is_dirac()) {
        return barycentric_if_point_inside(a,b,c, mu) ? f_t(1) : f_t(0);
    }

    // radius of circle, in canonical space, where we assume all the mass is (3σ contains 99%)
    static constexpr f_t L = 3;

    // fast paths
    {
        // in canonical
        a = to_canonical(a);
        b = to_canonical(b);
        c = to_canonical(c);

        // check tri AABB against circle
        if (m::min(a.x,b.x,c.x)>=L || m::max(a.x,b.x,c.x)<=-L ||
            m::min(a.y,b.y,c.y)>=L || m::max(a.y,b.y,c.y)<=-L)
            return 0;

        const bool a_in_C = util::is_point_in_circle(a, L);
        const bool b_in_C = util::is_point_in_circle(b, L);
        const bool c_in_C = util::is_point_in_circle(c, L);
        if (!a_in_C && !b_in_C && !c_in_C) {
            // triangle vertices outside circle
            // check edge intersection
            const bool intersects_ab = intersect::intersect_edge_circle(a*u::m, b*u::m, L*u::m).points>0;
            const bool intersects_ac = intersect::intersect_edge_circle(a*u::m, c*u::m, L*u::m).points>0;
            const bool intersects_bc = intersect::intersect_edge_circle(b*u::m, c*u::m, L*u::m).points>0;
            if (!intersects_ab && !intersects_ac && !intersects_bc) {
                const bool tri_contains_mu = util::is_point_in_triangle({ 0,0 }, a,b,c);
                // triangle contains circle? otherwise triangle is fully outside circle.
                return tri_contains_mu ? 1 : 0;
            }
        }
    }

    // slow path: analytic approximation for large triangles, more accurate quadrature for small ones
    // both are slow and not very accurate.
    // TODO: do better

    const auto min_len = m::min(m::length2(a-b), m::length2(a-c), m::length2(b-c));
    if (min_len<1e-3) {
        // quadrature step
        static constexpr f_t delta = .002;

        // a - bottom vertex
        if (b.y<a.y) std::swap(a,b);
        if (c.y<a.y) std::swap(a,c);

        const auto ab_dxdy = b.y==a.y ? m::inf : (b.x-a.x)/(b.y-a.y);
        const auto ac_dxdy = c.y==a.y ? m::inf : (c.x-a.x)/(c.y-a.y);
        const auto bc_dxdy = c.y==b.y ? m::inf : (c.x-b.x)/(c.y-b.y);

        f_t ret = 0;
        for (auto y = m::max(-L,a.y+delta/2); y<m::min(L,m::max(b.y,c.y)); y+=delta) {
            auto x0 = y<b.y ? ab_dxdy*(y-a.y) + a.x : bc_dxdy*(y-b.y) + b.x;
            auto x1 = y<c.y ? ac_dxdy*(y-a.y) + a.x : bc_dxdy*(y-b.y) + b.x;
            if (x0>x1) std::swap(x0,x1);
            for (auto x = m::max(-L,x0)+delta/2; x<m::min(L,x1); x+=delta)
                ret += m::exp(-(m::sqr(x)+m::sqr(y))/2);
        }

        assert(m::isfinite(ret) && ret>=0);

        return ret * m::inv_two_pi * m::sqr(f_t(delta));
    } else {
        // analytic approximation (expensive!)

        const auto T = mat2_t(b-a, c-a);
        const auto mu0 = m::inverse(T) * a;
        const auto A = m::transpose(T) * T;

        const auto detA = m::determinant(A);
        const auto Sxy = A[0][1];
        const auto Syy = A[1][1];

        if (Syy<=0 || detA<=0) return 0;

        const auto denom = 1/m::sqrt(2*Syy);
        const auto pa = detA * m::sqr(denom);
        const auto pb = mu0[0];
        const auto c0 = Sxy * denom;
        const auto d0 = (Sxy*mu0[0] + Syy*mu0[1])*denom;
        const auto q  = f_t(.5) / denom;

        const auto I0 = detail::I_gauss_erf(pa, pb, c0-q, d0+q);
        const auto I1 = detail::I_gauss_erf(pa, pb, c0, d0);

        assert(m::isfinite(denom) && m::isfinite(I0) && m::isfinite(I1));

        return m::inv_sqrt_pi/2 * m::abs(m::determinant(T) * denom) * m::max<f_t>(0,I0-I1);
    }
}

}
