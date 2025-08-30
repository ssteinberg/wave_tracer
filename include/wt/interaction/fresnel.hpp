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

namespace wt {

/**
 * @brief Reflects the direction vector w w.r.t. surface normal n. w is assumed to point away from the surface.
 */
inline auto reflect(const dir3_t& w, const dir3_t& n = { 0,0,1 }) noexcept {
    return dir3_t{ f_t(2)*vec3_t{ m::dot(w,n)*n } - vec3_t{ w } };
}


struct refract_ret_t {
    dir3_t t;
    f_t cost, eta_12;
    bool TIR = false;
};

/**
 * @brief Refracts the direction vector w w.r.t. surface normal n. w is assumed to point away from the surface.
 *        On total-internal-reflection returns TIR=true.
 * 
 * @param eta_12 Refractive-index ratio
 */
inline auto refract(f_t eta_12, const dir3_t& w, const dir3_t& n = { 0,0,1 }) noexcept {
    const auto wn = m::dot(w,n);
    eta_12 = wn>0 ? eta_12 : f_t(1)/eta_12;

    const auto cost2 = 1 - m::sqr(eta_12) * (1-m::sqr(wn));
    if (cost2>=0) {
        const auto cost = m::sqrt(cost2);
        const auto t = eta_12*(vec3_t{ wn*n } - vec3_t{ w }) - cost*(wn>=0?n:-n);

        return refract_ret_t{
            .t    = m::normalize(t),
            .cost = cost,
            .eta_12 = eta_12,
            .TIR  = false,
        };
    }

    return refract_ret_t{ .t = { 0,0,1 }, .eta_12 = eta_12, .TIR = true, };
}

struct fresnel_ret_t {
    dir3_t t;
    c_t eta_12;
    f_t Z;  // change of impedance
    c_t rs,rp;
    c_t ts,tp;
    f_t Ts,Tp;

    [[nodiscard]] inline bool TIR() const noexcept { return Ts==0 && Tp==0; }
    [[nodiscard]] inline auto Rs() const noexcept  { return 1-Ts; }
    [[nodiscard]] inline auto Rp() const noexcept  { return 1-Tp; }
};

/**
 * @brief Computes the Fresnel coefficients as well as refracted ray direction at an interface. w is assumed to point away from the surface.
 * 
 * @param eta_12 Refractive-index ratio
 */
inline fresnel_ret_t fresnel(c_t eta_12, const dir3_t& w, const dir3_t& n = { 0,0,1 }) noexcept {
    if (eta_12 == f_t(1)) {
        return {
            .t = -w,
            .eta_12 = eta_12,
            .Z = 1,
            .rs = 0, .rp = 0, 
            .ts = 1, .tp = 1, .Ts = 1, .Tp = 1,
        };
    }

    const auto abs_cosi = m::abs(m::dot(w,n));
    const auto refr = refract(std::real(eta_12), w, n);

    if (abs_cosi==0 || refr.TIR) {
        return {
            .t = { 0,0,1 },
            .eta_12 = refr.eta_12,
            .Z = 1,
            .rs = 1, .rp = 1, 
            .ts = 0, .tp = 0, .Ts = 0, .Tp = 0,
        };
    }

    const auto cost = refr.cost;
    eta_12 = refr.eta_12;

    const auto rs = (eta_12 * abs_cosi - cost) / (eta_12 * abs_cosi + cost);
    const auto rp = (abs_cosi - eta_12 * cost) / (abs_cosi + eta_12 * cost);
    const auto ts =  rs + c_t{ 1,0 };
    const auto tp = (rp + c_t{ 1,0 }) * eta_12;

    const auto Z = std::abs(cost/(eta_12*abs_cosi));

    return {
        .t = refr.t,
        .eta_12 = eta_12,
        .Z = Z,
        .rs = rs, .rp = rp,
        .ts = ts, .tp = tp,
        .Ts = m::min<f_t>(1, Z * std::norm(ts)), 
        .Tp = m::min<f_t>(1, Z * std::norm(tp)),
    };
}

struct fresnel_conductor_ret_t {
    c_t rs,rp;
};

/**
 * @brief Computes the Fresnel coefficients (reflection only) at a conductive interface. w is assumed to point away from the surface.
 * 
 * @param eta_12 (Complex-valued) refractive-index ratio
 */
inline fresnel_conductor_ret_t fresnel_reflection(c_t eta_12, const dir3_t& w, const dir3_t& n = { 0,0,1 }) {
    const auto wn = m::dot(w,n);
    if (eta_12==c_t{ 1,0 } || wn<0) {
        return {
            .rs = 0, .rp = 0,
        };
    }

    const auto t2 = c_t{ 1,0 } - (1-m::sqr(wn)) * m::sqr(eta_12);
    const auto t = m::sqrt(t2);
    const auto i = c_t{ wn,0 };

    return {
        .rs = (eta_12*i - t) / (eta_12*i + t),
        .rp = (i - eta_12*t) / (i + eta_12*t),
    };
}

}
