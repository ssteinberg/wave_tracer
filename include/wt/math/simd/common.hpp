/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cstdint>
#include <wt/math/common.hpp>
#include <wt/util/concepts.hpp>

namespace wt::simd {


/**
 * @brief Elements in a unitless wide vector can be treated as bool TRUE/FALSE.
 *        The signbit is used as the boolean flag, and this mask is used by different methods to **set** an element to TRUE value; **test** is always done via `wt::m::signbit`.
 */
template <FloatingPoint Fp>
struct scalar_logical_true_value {};
template <>
struct scalar_logical_true_value<double> {
    constexpr inline double operator()() const noexcept {
        return m::reinterpret_bits<double>(std::uint64_t(0xFFFFFFFFFFFFFFFF));
    }
};
template <>
struct scalar_logical_true_value<float> {
    constexpr inline float operator()() const noexcept {
        return m::reinterpret_bits<float>(std::uint32_t(0xFFFFFFFF));
    }
};

}