/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/common.hpp>
#include "common.hpp"

#define CERF_AS_CPP
#include <cerf.h>

namespace wt::utd {

static constexpr f_t utd_min_sin_beta = 1e-3;

/**
 * @brief The UTD `a±` function
 */
template <int sgn>
inline constexpr f_t UTDa(const Angle auto phi, f_t n) noexcept {
    static_assert(sgn==+1 || sgn==-1);

    const auto N = m::round((f_t)(sgn * m::pi + phi/u::ang::rad) * m::inv_two_pi / n);
    return 2 * m::sqr(m::cos(m::pi*n*N * u::ang::rad - phi/2));
}

/**
 * @brief The UTD `F` transition function
 */
inline constexpr c_t UTDF(f_t x) noexcept {
    const auto absx = m::abs(x);

    c_t result;
    if (absx<6) {
        const auto sqrt_x = m::sqrt(absx);
        const auto cerf = c_t{ cerfc(std::exp(c_t{ 0,m::pi_4 }) * sqrt_x) };
        result = c_t{ 1,1 } * m::sqrt_pi_2 * 
                 sqrt_x *
                 std::exp(c_t{ 0,absx }) *
                 cerf;
    } else {
        // fast approximation for large values
        const auto r = 1/(2*absx);
        const auto r2 = r*r;
        const auto r3 = r2*r;
        const auto r4 = r2*r2;
        result = f_t(1) + c_t{ 0,1 }*r - 3*r2 - c_t{ 0,15 }*r3 + 75*r4;
    }

    return x<0 ? std::conj(result) : result;
}

/**
* @brief Returns point on wedge that satisfies Fermat's principle, if such a point exists.
*/
[[nodiscard]] inline std::optional<pqvec3_t> wedge_edge_t::diffraction_point(
        const pqvec3_t& src,
        const pqvec3_t& dst) const noexcept {
    const auto& e = this->e();

    const auto sl = m::length(pqvec2_t{ m::dot(src - v, tff), m::dot(src - v, nff) });
    const auto dl = m::length(pqvec2_t{ m::dot(dst - v, tff), m::dot(dst - v, nff) });
    const auto dist = m::dot(e, src - v) + m::dot(dst - src, e) * sl/(sl + dl);

    if (m::abs(dist)>l/2)
        return std::nullopt;

    const auto p = v + e*dist;
    if (p==src || p==dst)
        return std::nullopt;
    return p;
}

/**
* @brief Returns point on wedge that satisfies Fermat's principle, if such a point exists.
*/
[[nodiscard]] inline std::optional<pqvec3_t> wedge_edge_t::diffraction_point(
        const pqvec3_t& src,
        const dir3_t& wo) const noexcept {
    const auto& e = this->e();

    const auto cos_beta = m::dot(wo,e);
    const auto sin_beta = m::sqrt(m::max<f_t>(0,1-m::sqr(cos_beta)));

    if (sin_beta<utd_min_sin_beta)
        return std::nullopt;

    const auto sl = m::length(pqvec2_t{ m::dot(src - v, tff), m::dot(src - v, nff) });
    const auto prj_src = v + m::dot(src-v,e)*e;
    const auto p = prj_src + sl * (cos_beta / sin_beta) * e;

    if (m::length2(p-v)>m::sqr(l/2))
        return std::nullopt;
    if (p==src)
        return std::nullopt;

    assert_iszero(m::sqrt(m::max<f_t>(0,1-m::sqr(cos_beta))) - sin_beta);
    assert_iszero<f_t>(m::dot(m::normalize(src-p),e) + cos_beta, 10);

    return p;
}

/**
* @brief The UTD wedge diffraction function. Does NOT account for the phase term exp(-i*k*ro).
*/
[[nodiscard]] inline UTD_ret_t wedge_edge_t::UTD(
        const Wavenumber auto k,
        const dir3_t& wi,
        const dir3_t& wo,
        const length_t ro) const noexcept {
    assert_iszero(m::dot(nff,tff));
    
    const auto& e = this->e();
    const auto n = 2 - (f_t)(alpha/u::ang::rad)*m::inv_pi;

    // build in/out transverse frames
    const auto& ti = -m::normalize(m::cross(e, -wi));
    const auto& bi =  m::normalize(m::cross(ti,-wi));
    const auto& to = -m::normalize(m::cross(e,  wo));
    const auto& bo =  m::normalize(m::cross(to, wo));

    // angles
    const auto sin_beta2 = m::max<f_t>(0, 1-m::sqr(m::dot(wi,e)));
    const auto sin_beta = m::sqrt(sin_beta2);
    const auto phii = m::atan2(m::dot(nff,wi), m::dot(tff,wi));
    const auto phio = m::atan2(m::dot(nff,wo), m::dot(tff,wo));

    // distance parameters
    const auto Li = ro * sin_beta2;
    const auto Lrn = Li, Lro = Li;

    // diffraction coefficients
    const auto a1 = UTDa<+1>(phii-phio, n);
    const auto a2 = UTDa<-1>(phii-phio, n);
    const auto a3 = UTDa<+1>(phii+phio, n);
    const auto a4 = UTDa<-1>(phii+phio, n);
    const auto F1 = UTDF(u::to_num(k * Li ) * a1);
    const auto F2 = UTDF(u::to_num(k * Li ) * a2);
    const auto F3 = UTDF(u::to_num(k * Lrn) * a3);
    const auto F4 = UTDF(u::to_num(k * Lro) * a4);
    const auto D1 = -m::cot((m::pi*u::ang::rad + (phii-phio)) / (2*n)) * F1;
    const auto D2 = -m::cot((m::pi*u::ang::rad - (phii-phio)) / (2*n)) * F2;
    const auto D3 = -m::cot((m::pi*u::ang::rad + (phii+phio)) / (2*n)) * F3;
    const auto D4 = -m::cot((m::pi*u::ang::rad - (phii+phio)) / (2*n)) * F4;

    const auto kro = u::to_num(k*ro);
    const auto D =
        1/(2*n*m::sqrt(kro)*sin_beta) * m::inv_sqrt_two_pi *
        std::exp(c_t{ 0,-m::pi_4 });

    const auto t1 = (f_t)(m::mod(phii+phio, m::pi_2) / u::ang::rad);
    const auto t2 = (f_t)(m::mod(phii-phio, m::pi_2) / u::ang::rad);
    const auto Ds = m::abs(t1)<f_t(1e-5) || m::abs(t2)<f_t(1e-5) ? 0 : D1+D2-(D3+D4);
    const auto Dh = m::abs(t1)<f_t(1e-5) || m::abs(t2)<f_t(1e-5) ? 0 : D1+D2+(D3+D4);

    assert(m::isfinite(Dh) && m::isfinite(Ds));

    return UTD_ret_t{
        .Ds = -D*Ds,
        .Dh = -D*Dh,
        .si = ti,
        .hi = bi,
        .so = to,
        .ho = bo,
    };
}

}
