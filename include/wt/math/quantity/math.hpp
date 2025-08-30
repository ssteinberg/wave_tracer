/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/quantity/framework.hpp>
#include <wt/math/quantity/defs.hpp>
#include <wt/math/quantity/concepts.hpp>
#include <wt/util/concepts.hpp>

namespace wt {


// common conversions

/**
 * @brief Converts wavelength to wavenumber.
 */
constexpr inline Wavenumber auto wavelen_to_wavenum(const Wavelength auto lambda) noexcept {
    return m::two_pi / lambda;
}
/**
 * @brief Converts wavenumber to wavelength.
 */
constexpr inline Wavelength auto wavenum_to_wavelen(const Wavenumber auto k) noexcept {
    return m::two_pi / k;
}

/**
 * @brief Converts EM radiation frequency to wavelength (in vacuum).
 */
constexpr inline Wavelength auto freq_to_wavelen(const Frequency auto freq) noexcept {
    constexpr auto c = f_t(1) * siconstants::speed_of_light_in_vacuum;
    return c / freq;
}
/**
 * @brief Converts EM radiation frequency to wavenumber (in vacuum).
 */
constexpr inline Wavenumber auto freq_to_wavenum(const Frequency auto freq) noexcept {
    constexpr auto c = siconstants::speed_of_light_in_vacuum;
    return m::two_pi / c * freq;
}

/**
 * @brief Converts wavenumber (in vacuum) to EM radiation frequency.
 */
constexpr inline Frequency auto wavenum_to_freq(const Wavenumber auto k) noexcept {
    constexpr auto c = siconstants::speed_of_light_in_vacuum;
    return m::inv_two_pi * c * k;
}


namespace m {

constexpr inline auto sqrt(const Quantity auto q) noexcept {
    return mp_units::sqrt(q);
}
template<QuantityRefOf<dimensionless> auto R, typename Rep>
constexpr inline auto exp(const quantity<R, Rep>& q) noexcept {
    return mp_units::exp(q);
}
constexpr inline auto abs(const Quantity auto q) noexcept {
    return mp_units::abs(q);
}
constexpr inline auto inverse(const Quantity auto q) noexcept {
    return mp_units::inverse(q);
}
constexpr inline auto floor(const Quantity auto q) noexcept {
    return mp_units::floor(q);
}
constexpr inline auto ceil(const Quantity auto q) noexcept {
    return mp_units::ceil(q);
}
constexpr inline auto round(const Quantity auto q) noexcept {
    return mp_units::round(q);
}

template <Quantity Q>
constexpr inline auto mod(const Q q, const typename Q::rep& s) noexcept {
    using T = Q::rep;
    return glm::mod<T>(q.numerical_value_in(q.unit),s) * (T(1) * q.unit);
}

/**
 * @brief Returns `+1` when`q>0`, `0` when `q==0`, and `-1` when `q<0`.
 */
template <Quantity Q>
constexpr inline auto sign(const Q& q) noexcept {
    return glm::sign(q.numerical_value_in(q.unit));
}
/**
 * @brief Returns TRUE if the sign bit of a floating point is set.
 */
template <Quantity Q>
    requires (FloatingPoint<typename Q::rep>)
constexpr inline auto signbit(const Q& q) noexcept {
    return std::signbit(q.numerical_value_in(q.unit));
}

constexpr inline auto is_lt(const Quantity auto q1, const Quantity auto q2) noexcept {
    return is_lt_zero(q1-q2);
}
constexpr inline auto is_lteq(const Quantity auto q1, const Quantity auto q2) noexcept {
    return is_lteq_zero(q1-q2);
}
constexpr inline auto is_gt(const Quantity auto q1, const Quantity auto q2) noexcept {
    return is_gt_zero(q1-q2);
}
constexpr inline auto is_gteq(const Quantity auto q1, const Quantity auto q2) noexcept {
    return is_gteq_zero(q1-q2);
}

template <Quantity Q>
constexpr inline auto min(const Q& q1, const Q& q2) noexcept {
    return is_lt(q1,q2) ? q1 : q2;
}
template <Quantity Q>
constexpr inline auto max(const Q& q1, const Q& q2) noexcept {
    return is_gt(q1,q2) ? q1 : q2;
}
template <Quantity Q>
constexpr inline auto min(const Q& q1, const Q& q2, const Q& q3) noexcept {
    return min(q1,min(q2,q3));
}
template <Quantity Q>
constexpr inline auto max(const Q& q1, const Q& q2, const Q& q3) noexcept {
    return max(q1,max(q2,q3));
}
template <Quantity Q>
constexpr inline auto min(const Q& q1, const Q& q2, const Q& q3, const Q& q4) noexcept {
    return min(min(q1,q2),min(q3,q4));
}
template <Quantity Q>
constexpr inline auto max(const Q& q1, const Q& q2, const Q& q3, const Q& q4) noexcept {
    return max(max(q1,q2),max(q3,q4));
}

template <Quantity Q>
constexpr inline auto clamp(const Q& v, 
                            const Q& minv, 
                            const Q& maxv) noexcept {
    return min(max(v, minv), maxv);
}

template <Quantity Q, NumericOrBool S>
constexpr inline auto mix(const Q& a, 
                          const Q& b, 
                          const S& x) noexcept {
    if (x==S(0)) return a;
    if (x==S(1)) return b;
    return a * (S(1) - x) + b * x;
}
template <Quantity Q>
constexpr inline auto mix(const Q& a, 
                          const Q& b, 
                          const bool x) noexcept {
    return x ? b : a;
}

constexpr inline auto iszero(const Quantity auto q) noexcept {
    return q==q.zero();
}
constexpr inline auto isfinite(const Quantity auto q) noexcept {
    return mp_units::isfinite(q);
}
constexpr inline auto isnan(const Quantity auto q) noexcept {
    return mp_units::isnan(q);
}

constexpr inline auto isltzero(const Quantity auto q) noexcept {
    return mp_units::is_lt_zero(q);
}
constexpr inline auto islteqzero(const Quantity auto q) noexcept {
    return mp_units::is_lteq_zero(q);
}
constexpr inline auto isgtzero(const Quantity auto q) noexcept {
    return mp_units::is_gt_zero(q);
}
constexpr inline auto isgteqzero(const Quantity auto q) noexcept {
    return mp_units::is_gteq_zero(q);
}


constexpr inline auto sin(const Angle auto q) noexcept {
    return u::to_num(mp_units::angular::sin(q));
}
constexpr inline auto cos(const Angle auto q) noexcept {
    return u::to_num(mp_units::angular::cos(q));
}
constexpr inline auto tan(const Angle auto q) noexcept {
    return u::to_num(mp_units::angular::tan(q));
}
constexpr inline auto cot(const Angle auto q) noexcept {
    using T = decltype(q)::rep;
    return u::to_num(T(1)/mp_units::angular::tan(q));
}

constexpr inline Angle auto asin(const Numeric auto x) noexcept {
    return mp_units::angular::asin(x * u::one);
}
constexpr inline Angle auto acos(const Numeric auto x) noexcept {
    return mp_units::angular::acos(x * u::one);
}
constexpr inline Angle auto atan(const Numeric auto x) noexcept {
    return mp_units::angular::atan(x * u::one);
}
template <Numeric T>
constexpr inline Angle auto atan2(const T y, const T x) noexcept {
    return mp_units::angular::atan2(y*u::one, x*u::one);
}
constexpr inline Angle auto acot(const Numeric auto x) noexcept {
    return mp_units::angular::atan((decltype(x)(1)/x) * u::one);
}


}

constexpr inline auto operator<(const Quantity auto o1, const Quantity auto o2) noexcept {
    return m::is_lt(o1,o2);
}
constexpr inline auto operator<=(const Quantity auto o1, const Quantity auto o2) noexcept {
    return m::is_lteq(o1,o2);
}
constexpr inline auto operator>(const Quantity auto o1, const Quantity auto o2) noexcept {
    return m::is_gt(o1,o2);
}
constexpr inline auto operator>=(const Quantity auto o1, const Quantity auto o2) noexcept {
    return m::is_gteq(o1,o2);
}

}

#include <wt/math/quantity/quantity_vector.hpp>
#include <wt/math/quantity/quantity_limits.hpp>
