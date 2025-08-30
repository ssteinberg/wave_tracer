/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <concepts>

#include <wt/math/common.hpp>

namespace wt::m {

template<std::unsigned_integral T>
[[nodiscard]] inline auto unorm_to_fp(T x) noexcept {
    return m::clamp<f_t>(
        f_t(x) / f_t(limits<T>::max()),
        0, 1);
}
template<std::signed_integral T>
[[nodiscard]] inline auto snorm_to_fp(T x) noexcept {
    return m::clamp<f_t>(
        f_t(x) / f_t(limits<T>::max()),
        -1, 1);
}

template<std::unsigned_integral T>
[[nodiscard]] inline auto fp_to_unorm(f_t x) noexcept {
    x = m::clamp<f_t>(x,0,1);
    return T(m::min<f_t>(m::round(x * limits<T>::max()), 
                         limits<T>::max()-f_t(.5)));
}
template<std::signed_integral T>
[[nodiscard]] inline auto fp_to_snorm(f_t x) noexcept {
    x = m::clamp<f_t>(x,-1,1);
    return T(m::clamp<f_t>(m::round(x * limits<T>::max()), 
                           limits<T>::min()+f_t(.5),
                           limits<T>::max()-f_t(.5)));
}

}
