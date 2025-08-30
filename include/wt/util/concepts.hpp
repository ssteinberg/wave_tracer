/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <type_traits>
#include <complex>

#include <wt/util/type_traits.hpp>

#include <wt/math/glm.hpp>
#include <wt/math/quantity/concepts.hpp>

namespace wt {

/**
 * @brief Concept for enums
 */
template <typename T>
concept Enum = requires(T param) {
    requires std::is_enum_v<T>;
};
/**
 * @brief Concept for scoped enums
 */
template <typename T>
concept ScopedEnum = requires(T param) {
    requires is_scoped_enum_v<T>;
};

/**
 * @brief Concept for floating-point types.
 */
template <typename T>
concept FloatingPoint = std::floating_point<T>;

/**
 * @brief Concept for integer types.
 */
template <typename T>
concept Integer = std::integral<T>;

/**
 * @brief Concept for complex numbers.
 */
template <typename T>
concept Complex = std::is_same_v<T,std::complex<float>> || std::is_same_v<T,std::complex<double>>;

/**
 * @brief Concept for numeric types: floating-points and integers. Does not include boolean or pointer types.
 */
template <typename T>
concept Numeric = requires(T param) {
    requires std::is_integral_v<T> || std::is_floating_point_v<T>;
    requires std::is_arithmetic_v<decltype(param+1)>;
    requires !std::is_same_v<bool, T>;
    requires !std::is_pointer_v<T>;
};

/**
 * @brief Concept for floating-points, integers and complex types. Does not include boolean or pointer types.
 */
template <typename T>
concept NumericOrComplex = Numeric<T> || Complex<T>;

/**
 * @brief Concept for numeric-like types: Numeric or Quantity.
 */
template<typename T>
concept Scalar = Numeric<T> || Quantity<T> || QuantityPoint<T>;

/**
 * @brief Concept for real and complex numeric-like types: Numeric, Complex or Quantity.
 */
template<typename T>
concept CScalar = Numeric<T> || Quantity<T> || QuantityPoint<T> || Complex<T>;

/**
 * @brief Concept for numeric-like types and units: Numeric, Quantity, QuantityRef, or Unit.
 */
template<typename T>
concept ScalarOrUnit = Numeric<T> || Quantity<T> || QuantityPoint<T> || QuantityRef<T> || Unit<T>;

/**
 * @brief Concept for quantities and units: Quantity, QuantityRef, or Unit.
 */
template<typename T>
concept QuantityOrUnit = Quantity<T> || QuantityPoint<T> || QuantityRef<T> || Unit<T>;

/**
 * @brief Concept for floating-points, integers and boolean types.
 */
template <typename T>
concept NumericOrBool = Numeric<T> || std::is_same_v<T,bool>;

/**
 * @brief Concept for numeric-like types: Numeric or Quantity; or bool.
 */
template <typename T>
concept ScalarOrBool = Scalar<T> || std::is_same_v<T,bool>;

/**
 * @brief Concept for real and complex numeric-like types: Numeric, Complex or Quantity; or bool.
 */
template <typename T>
concept CScalarOrBool = CScalar<T> || std::is_same_v<T,bool>;


namespace detail {

template <std::size_t N, typename T>
void to_base_specialization_of_glm_vector(const glm::vec<N, T>*);
template<typename T>
constexpr bool is_derived_from_specialization_of_glm_vector =
    requires(T* type) { to_base_specialization_of_glm_vector(type); };

}

/**
 * @brief Concept for generic unitless vectors.
 */
template <typename T>
concept NumericVector = detail::is_derived_from_specialization_of_glm_vector<T>;

}
