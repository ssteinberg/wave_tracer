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
#include <cassert>

namespace wt::colourspace {

/**
 * @brief Computes the existant radiance of a blackbody radiator.
 * 
 * @param lambda wavelength
 * @param T temperature
 * @return f_t spectral radiance
 */
inline f_t planck_blackbody(const Wavelength auto& lambda, 
                            const Temperature auto& T) noexcept {
    if (T.quantity_from_zero()<=zero)
        return 0;

    constexpr auto c =  1. * siconstants::speed_of_light_in_vacuum;    // 299792458. * u::m/u::s;
    constexpr auto h =  1. * siconstants::planck_constant;             // 6.62606957e-34 * u::J * u::s;
    constexpr auto kB = 1. * siconstants::boltzmann_constant;          // 1.380649e-23 * u::J/u::K;

    // first and second radiation constants
    constexpr auto c1 = 2*h*c*c;
    constexpr auto c2 = h*c / kB; // 1.438776877e-2

    if (lambda==zero)
        return 0;

    const auto l5 = lambda*m::sqr(lambda*lambda);
    const auto x  = u::to_num(c2 / (lambda*T.quantity_from_zero()));
    const auto Le = c1 / (l5 * (m::exp(x)-1));
    assert(m::isfinite(Le));

    // Return emitted radiance for blackbody at wavelength lambda.
    const auto ret = Le.numerical_value_in(u::J/u::s/pow<2>(u::m)/u::mm);
    // scale, to make values more inline with emitter db quantities
    return (f_t)ret * 1e-10;
}

/**
 * @brief Computes the Planckian locus in XYZ colourspace, i.e. the colour of a blackbody radiator.
 *        Uses a cubic spline approximation. "Design of Advanced Color Temperature Control System for HDTV Applications", Kang et al. December 2002.
 * 
 * @param T temperature
 * @return vec3_t XYZ tristimulus colour
 */
inline vec3_t planckian_locus(const Temperature auto& T) noexcept {
    const bool s1 = point<u::K>(1500) <= T && T < point<u::K>(2222);
    const bool s2 = point<u::K>(2222) <= T && T < point<u::K>(4000);
    const bool s3 = point<u::K>(4000) <= T && T < point<u::K>(25000);

    if (!s1 && !s2 && !s3)
        return {};

    const double rT = 1./(T.in(u::K).quantity_from_zero().numerical_value_in(u::K));
    const double t1 = 1e3*rT;
    const double t2 = 1e6*rT*rT;
    const double t3 = 1e9*rT*rT*rT;

    static constexpr double c13 = -0.2661239;
    static constexpr double c12 = -0.2343589;
    static constexpr double c11 =  0.8776956;
    static constexpr double c10 =  0.179910;
    static constexpr double c23 = -3.0258469;
    static constexpr double c22 =  2.1070379;
    static constexpr double c21 =  0.2226347;
    static constexpr double c20 =  0.240390;
    static constexpr double d13 = -1.1063814;
    static constexpr double d12 = -1.34811020;
    static constexpr double d11 =  2.18555832;
    static constexpr double d10 = -0.20219683;
    static constexpr double d23 = -0.9549476;
    static constexpr double d22 = -1.3741859;
    static constexpr double d21 =  2.09137015;
    static constexpr double d20 = -0.16748867;
    static constexpr double d33 =  3.0817580;
    static constexpr double d32 = -5.87338670;
    static constexpr double d31 =  3.75112997;
    static constexpr double d30 = -0.37001483;

    const auto x = s1||s2 ? c13*t3 + c12*t2 + c11*t1 + c10 :
                            c23*t3 + c22*t2 + c21*t1 + c20;

    const auto x2 = x*x;
    const auto x3 = x2*x;

    const auto y = s1 ? d13*x3 + d12*x2 + d11*x + d10 :
                   s2 ? d23*x3 + d22*x2 + d21*x + d20 :
                        d33*x3 + d32*x2 + d31*x + d30;
    
    assert(m::isfinite(x) && m::isfinite(y) && y>0);

    // to XYZ with Y=1
    const auto ry = f_t(1)/f_t(y);
    return y==0 ? vec3_t{} :
                  vec3_t{ f_t(x)*ry, 1., f_t(1-x-y)*ry };
}

}
