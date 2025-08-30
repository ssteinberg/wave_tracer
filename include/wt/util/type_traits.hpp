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


namespace wt {

namespace detail {
void test_conversion(...);          // selected when E is complete and scoped
void test_conversion(int) = delete; // selected when E is complete and unscoped
 
template<class E>
concept is_scoped_enum_impl =
    std::is_enum_v<E> &&                        // checked first
    requires { detail::test_conversion(E{}); }; // ill-formed before overload resolution
                                                // when E is incomplete
}

template<class T>
struct is_scoped_enum : std::bool_constant<detail::is_scoped_enum_impl<T>> {};
template<class T>
static constexpr auto is_scoped_enum_v = is_scoped_enum<T>::value;


}
