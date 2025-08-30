/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/defs.hpp>
#include <wt/util/concepts.hpp>

namespace wt {


/**
 * @brief General-purpose placeholder for 0 works with NumericOrComplex as well as Quantity types.
 */
struct zero_t {};
inline constexpr zero_t zero;


template <NumericOrComplex T>
inline constexpr bool operator==(const T& s, zero_t) noexcept {
    return s == (T)0;
}
template <NumericOrComplex T>
inline constexpr bool operator==(zero_t, const T& s) noexcept {
    return (T)0 == s;
}
template <NumericOrComplex T>
inline constexpr bool operator!=(const T& s, zero_t) noexcept {
    return s != (T)0;
}
template <NumericOrComplex T>
inline constexpr bool operator!=(zero_t, const T& s) noexcept {
    return (T)0 != s;
}
template <NumericOrComplex T>
inline constexpr bool operator<(const T& s, zero_t) noexcept {
    return s < (T)0;
}
template <NumericOrComplex T>
inline constexpr bool operator<(zero_t, const T& s) noexcept {
    return (T)0 < s;
}
template <NumericOrComplex T>
inline constexpr bool operator<=(const T& s, zero_t) noexcept {
    return s <= (T)0;
}
template <NumericOrComplex T>
inline constexpr bool operator<=(zero_t, const T& s) noexcept {
    return (T)0 <= s;
}
template <NumericOrComplex T>
inline constexpr bool operator>(const T& s, zero_t) noexcept {
    return s > (T)0;
}
template <NumericOrComplex T>
inline constexpr bool operator>(zero_t, const T& s) noexcept {
    return (T)0 > s;
}
template <NumericOrComplex T>
inline constexpr bool operator>=(const T& s, zero_t) noexcept {
    return s >= (T)0;
}
template <NumericOrComplex T>
inline constexpr bool operator>=(zero_t, const T& s) noexcept {
    return (T)0 >= s;
}

template <Quantity Q>
inline constexpr bool operator==(const Q& s, zero_t) noexcept {
    return s == Q::zero();
}
template <Quantity Q>
inline constexpr bool operator==(zero_t, const Q& s) noexcept {
    return Q::zero() == s;
}
template <Quantity Q>
inline constexpr bool operator!=(const Q& s, zero_t) noexcept {
    return s != Q::zero();
}
template <Quantity Q>
inline constexpr bool operator!=(zero_t, const Q& s) noexcept {
    return Q::zero() != s;
}
template <Quantity Q>
inline constexpr bool operator<(const Q& s, zero_t) noexcept {
    return s < Q::zero();
}
template <Quantity Q>
inline constexpr bool operator<(zero_t, const Q& s) noexcept {
    return Q::zero() < s;
}
template <Quantity Q>
inline constexpr bool operator<=(const Q& s, zero_t) noexcept {
    return s <= Q::zero();
}
template <Quantity Q>
inline constexpr bool operator<=(zero_t, const Q& s) noexcept {
    return Q::zero() <= s;
}
template <Quantity Q>
inline constexpr bool operator>(const Q& s, zero_t) noexcept {
    return s > Q::zero();
}
template <Quantity Q>
inline constexpr bool operator>(zero_t, const Q& s) noexcept {
    return Q::zero() > s;
}
template <Quantity Q>
inline constexpr bool operator>=(const Q& s, zero_t) noexcept {
    return s >= Q::zero();
}
template <Quantity Q>
inline constexpr bool operator>=(zero_t, const Q& s) noexcept {
    return Q::zero() >= s;
}

}
