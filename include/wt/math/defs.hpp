/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cmath>
#include <complex>
#include <numbers>
#include <limits>

#include <wt/math/glm.hpp>

#include <wt/util/concepts.hpp>


namespace wt {

/**
 * @brief General floating point type.
 *        Configured via the `_FLOAT_TYPE` compile-time define. Defaults to 32-bit `float`.
 */
using f_t = _FLOAT_TYPE;
/**
 * @brief General complex number type.
 */
using c_t = std::complex<f_t>;


namespace m {

[[maybe_unused]] static constexpr auto inf = std::numeric_limits<f_t>::infinity();

[[maybe_unused]] static constexpr auto pi           = std::numbers::pi_v<f_t>;
[[maybe_unused]] static constexpr auto two_pi       = f_t(2. * std::numbers::pi);
[[maybe_unused]] static constexpr auto three_pi     = f_t(3. * std::numbers::pi);
[[maybe_unused]] static constexpr auto four_pi      = f_t(4. * std::numbers::pi);
[[maybe_unused]] static constexpr auto five_pi      = f_t(5. * std::numbers::pi);
[[maybe_unused]] static constexpr auto pi_2         = f_t(std::numbers::pi / 2.);
[[maybe_unused]] static constexpr auto pi_3         = f_t(std::numbers::pi / 3.);
[[maybe_unused]] static constexpr auto pi_4         = f_t(std::numbers::pi / 4.);
[[maybe_unused]] static constexpr auto pi_5         = f_t(std::numbers::pi / 5.);
[[maybe_unused]] static constexpr auto inv_pi       = std::numbers::inv_pi_v<f_t>;
[[maybe_unused]] static constexpr auto inv_two_pi   = f_t(std::numbers::inv_pi / 2.);
[[maybe_unused]] static constexpr auto inv_three_pi = f_t(std::numbers::inv_pi / 3.);
[[maybe_unused]] static constexpr auto inv_four_pi  = f_t(std::numbers::inv_pi / 4.);
[[maybe_unused]] static constexpr auto inv_five_pi  = f_t(std::numbers::inv_pi / 5.);

[[maybe_unused]] static constexpr auto sqrt_two         = std::numbers::sqrt2_v<f_t>;
[[maybe_unused]] static constexpr auto sqrt_three       = std::numbers::sqrt3_v<f_t>;
[[maybe_unused]] static const auto sqrt_five            = f_t(std::sqrt(5.));
[[maybe_unused]] static constexpr auto inv_sqrt_two     = f_t(1. / std::numbers::sqrt2);
[[maybe_unused]] static constexpr auto inv_sqrt_three   = std::numbers::inv_sqrt3_v<f_t>;
[[maybe_unused]] static const auto inv_sqrt_five        = f_t(1. / std::sqrt(5.));
[[maybe_unused]] static const auto sqrt_pi              = f_t(std::sqrt(std::numbers::pi));
[[maybe_unused]] static const auto sqrt_two_pi          = f_t(std::sqrt(2*std::numbers::pi));
[[maybe_unused]] static const auto sqrt_three_pi        = f_t(std::sqrt(3*std::numbers::pi));
[[maybe_unused]] static const auto sqrt_five_pi         = f_t(std::sqrt(5*std::numbers::pi));
[[maybe_unused]] static const auto sqrt_pi_2            = f_t(std::sqrt(std::numbers::pi/2));
[[maybe_unused]] static const auto sqrt_pi_3            = f_t(std::sqrt(std::numbers::pi/3));
[[maybe_unused]] static const auto sqrt_pi_4            = f_t(std::sqrt(std::numbers::pi/4));
[[maybe_unused]] static const auto sqrt_pi_5            = f_t(std::sqrt(std::numbers::pi/5));
[[maybe_unused]] static const auto inv_sqrt_pi          = f_t(1. / std::sqrt(std::numbers::pi));
[[maybe_unused]] static const auto inv_sqrt_two_pi      = f_t(1. / std::sqrt(2*std::numbers::pi));
[[maybe_unused]] static const auto inv_sqrt_three_pi    = f_t(1. / std::sqrt(3*std::numbers::pi));
[[maybe_unused]] static const auto inv_sqrt_pi_2        = f_t(1. / std::sqrt(std::numbers::pi/2));
[[maybe_unused]] static const auto inv_sqrt_pi_3        = f_t(1. / std::sqrt(std::numbers::pi/3));
[[maybe_unused]] static const auto inv_sqrt_pi_4        = f_t(1. / std::sqrt(std::numbers::pi/4));

[[maybe_unused]] static constexpr auto golden_ratio     = f_t(1.618033988749895);

}


template <std::size_t N, Numeric T>
using vec = glm::vec<N,T>;
template <std::size_t N>
using bvec = glm::vec<N,bool>;

template <std::size_t N,std::size_t M, Numeric T>
using mat = glm::mat<N,M,T>;

template <Numeric T>
using mat2 = mat<2,2,T>;
template <Numeric T>
using mat3 = mat<3,3,T>;
template <Numeric T>
using mat4 = mat<4,4,T>;
template <Numeric T>
using mat2x3 = mat<2,3,T>;
template <Numeric T>
using mat2x4 = mat<2,4,T>;
template <Numeric T>
using mat3x4 = mat<3,4,T>;
template <Numeric T>
using mat3x2 = mat<3,2,T>;
template <Numeric T>
using mat4x2 = mat<4,2,T>;
template <Numeric T>
using mat4x3 = mat<4,3,T>;

template <Numeric T>
using vec1 = vec<1,T>;
template <Numeric T>
using vec2 = vec<2,T>;
template <Numeric T>
using vec3 = vec<3,T>;
template <Numeric T>
using vec4 = vec<4,T>;

using mat2_t = mat2<f_t>;
using mat3_t = mat3<f_t>;
using mat4_t = mat4<f_t>;
using mat2x3_t = mat2x3<f_t>;
using mat2x4_t = mat2x4<f_t>;
using mat3x4_t = mat3x4<f_t>;
using mat3x2_t = mat3x2<f_t>;
using mat4x2_t = mat4x2<f_t>;
using mat4x3_t = mat4x3<f_t>;

using mat2d_t = mat2<double>;
using mat3d_t = mat3<double>;
using mat4d_t = mat4<double>;
using mat2x3d_t = mat2x3<double>;
using mat2x4d_t = mat2x4<double>;
using mat3x4d_t = mat3x4<double>;
using mat3x2d_t = mat3x2<double>;
using mat4x2d_t = mat4x2<double>;
using mat4x3d_t = mat4x3<double>;

using vec1_t = vec1<f_t>;
using vec2_t = vec2<f_t>;
using vec3_t = vec3<f_t>;
using vec4_t = vec4<f_t>;

using vec1d_t = vec1<double>;
using vec2d_t = vec2<double>;
using vec3d_t = vec3<double>;
using vec4d_t = vec4<double>;

using vec1b_t = bvec<1>;
using vec2b_t = bvec<2>;
using vec3b_t = bvec<3>;
using vec4b_t = bvec<4>;

using vec1i8_t = vec1<std::int8_t>;
using vec2i8_t = vec2<std::int8_t>;
using vec3i8_t = vec3<std::int8_t>;
using vec4i8_t = vec4<std::int8_t>;
using vec1u8_t = vec1<std::uint8_t>;
using vec2u8_t = vec2<std::uint8_t>;
using vec3u8_t = vec3<std::uint8_t>;
using vec4u8_t = vec4<std::uint8_t>;

using vec1i16_t = vec1<std::int16_t>;
using vec2i16_t = vec2<std::int16_t>;
using vec3i16_t = vec3<std::int16_t>;
using vec4i16_t = vec4<std::int16_t>;
using vec1u16_t = vec1<std::uint16_t>;
using vec2u16_t = vec2<std::uint16_t>;
using vec3u16_t = vec3<std::uint16_t>;
using vec4u16_t = vec4<std::uint16_t>;

using vec1i32_t = vec1<std::int32_t>;
using vec2i32_t = vec2<std::int32_t>;
using vec3i32_t = vec3<std::int32_t>;
using vec4i32_t = vec4<std::int32_t>;
using vec1u32_t = vec1<std::uint32_t>;
using vec2u32_t = vec2<std::uint32_t>;
using vec3u32_t = vec3<std::uint32_t>;
using vec4u32_t = vec4<std::uint32_t>;

using vec1i64_t = vec1<std::int64_t>;
using vec2i64_t = vec2<std::int64_t>;
using vec3i64_t = vec3<std::int64_t>;
using vec4i64_t = vec4<std::int64_t>;
using vec1u64_t = vec1<std::uint64_t>;
using vec2u64_t = vec2<std::uint64_t>;
using vec3u64_t = vec3<std::uint64_t>;
using vec4u64_t = vec4<std::uint64_t>;

}
