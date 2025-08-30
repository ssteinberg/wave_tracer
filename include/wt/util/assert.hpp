/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cassert>
#include <type_traits>
#include <utility>

#include <wt/util/concepts.hpp>
#include <wt/math/defs.hpp>

namespace wt {

template <FloatingPoint Fp>
static constexpr Fp f_t_assert_tolerance = std::is_same_v<Fp, float> ? 1e-6 : 1e-9;

/**
 * @brief Asserts that `expression' is 0, up to fp tolerance.
 */
template <FloatingPoint Fp>
inline void assert_iszero(Fp expression, Fp tolerance_scale = Fp(1)) noexcept {
    assert(glm::abs(expression) < f_t_assert_tolerance<Fp>*tolerance_scale);
}

/**
 * @brief Asserts that `expression' is NOT 0, up to fp tolerance.
 */
template <FloatingPoint Fp>
inline void assert_isnotzero(Fp expression, Fp tolerance_scale = Fp(1)) noexcept {
    assert(glm::abs(expression) >= f_t_assert_tolerance<Fp>*tolerance_scale);
}

template <FloatingPoint Fp, std::size_t Dim>
inline void assert_unit_vector(const vec<Dim,Fp>& v, Fp tolerance_scale = Fp(1)) noexcept {
    assert_iszero<Fp>(glm::length(v)-1, tolerance_scale);
}
template <FloatingPoint Fp, typename... Ts>
inline void assert_unit_vectors(Ts&&... ts) noexcept {
    (assert_unit_vector<Fp>(std::forward<Ts>(ts)), ...);
}

template <FloatingPoint Fp, std::size_t Dim>
inline void assert_orthogonal_vectors(const vec<Dim,Fp>& v1, const vec<Dim,Fp>& v2,
                                      Fp tolerance_scale = Fp(1)) noexcept {
    assert_iszero<Fp,tolerance_scale>(glm::dot(v1,v2), tolerance_scale);
}
template <FloatingPoint Fp, std::size_t Dim>
inline void assert_orthogonal_unit_vectors(const vec<Dim,Fp>& v1, const vec<Dim,Fp>& v2,
                                           Fp tolerance_scale = Fp(1)) noexcept {
    assert_unit_vectors<Fp>(v1,v2, tolerance_scale);
    assert_orthogonal_vectors<Fp>(v1,v2, tolerance_scale);
}

}
