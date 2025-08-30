/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/quantity/concepts.hpp>

namespace wt {

template <typename T>
struct is_quantity {
    static constexpr bool value = false;
};
template <Quantity Q>
struct is_quantity<Q> {
    static constexpr bool value = true;
};
template <typename T>
constexpr inline auto is_quantity_v = is_quantity<T>::value;

template <typename T>
struct is_quantity_point {
    static constexpr bool value = false;
};
template <QuantityPoint Q>
struct is_quantity_point<Q> {
    static constexpr bool value = true;
};
template <typename T>
constexpr inline auto is_quantity_point_v = is_quantity_point<T>::value;

}
