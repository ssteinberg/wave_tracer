/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <array>

namespace wt {

namespace detail {

template <typename T, std::size_t N, std::size_t... Ns>
struct array_generator {
    using type = std::array<typename array_generator<T, Ns...>::type, N>;
};
template <typename T, std::size_t N>
struct array_generator<T, N> {
    using type = std::array<T, N>;
};

}

template <typename T, std::size_t... Ns>
using array_t = detail::array_generator<T, Ns...>::type;

}
