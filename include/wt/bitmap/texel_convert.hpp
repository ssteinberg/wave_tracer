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
#include <type_traits>

#include <wt/math/common.hpp>
#include <wt/math/norm_integers.hpp>

namespace wt::bitmap {

template <class T>
concept texel = std::integral<T> || FloatingPoint<T>;

template <class T>
struct is_texel { static constexpr bool value = false; };
template <texel T>
struct is_texel<T> { static constexpr bool value = true; };
template <typename T>
static constexpr bool is_texel_v = is_texel<T>::value;


template <texel Dst, texel Src>
inline auto convert_texel(const Src src) noexcept {
    if constexpr (std::is_same_v<Src, Dst>)
        return src;

    f_t fsrc;
    if constexpr (std::is_floating_point_v<Src>)
        fsrc = f_t(src);
    else if constexpr (std::is_signed_v<Src>)
        fsrc = m::snorm_to_fp(src);
    else if constexpr (std::is_unsigned_v<Src>)
        fsrc = m::unorm_to_fp(src);
    else 
        static_assert(false);

    Dst dst;
    if constexpr (std::is_floating_point_v<Dst>)
        dst = fsrc;
    else if constexpr (std::is_signed_v<Dst>)
        dst = m::fp_to_snorm<Dst>(fsrc);
    else if constexpr (std::is_unsigned_v<Dst>)
        dst = m::fp_to_unorm<Dst>(fsrc);
    else 
        static_assert(false);

    return dst;
}

}
