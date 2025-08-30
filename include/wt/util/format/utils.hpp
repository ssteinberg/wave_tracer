/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <algorithm>

#include <wt/math/common.hpp>
#include <wt/util/concepts.hpp>

#include <string>
#include <string_view>
#include <cctype>
#include <bit>

namespace wt::format {

/**
 * @brief Trims prefix and suffix from the string.
 * @param trimchars characters to trim (defaults to whitespace and new line characters)
 */
template <bool trim_prefix = true, bool trim_suffix = true>
constexpr inline std::string trim(std::string_view sv, const char* trimchars = " \t\v\r\n") noexcept {
    if constexpr (trim_prefix)
        sv.remove_prefix( std::min(sv.find_first_not_of(trimchars), sv.size()));
    if constexpr (trim_suffix)
        sv = sv.substr( 0,std::min(sv.find_last_not_of(trimchars)+1, sv.size()));

    return std::string{ sv };
}

/**
 * @brief Finds a matching closing bracket
 */
template <char open='(', char close=')'>
constexpr inline std::size_t find_closing_bracket(std::string_view sv, std::size_t pos=0) noexcept {
    if (sv.empty() || sv[pos]!=open)
        return std::string::npos;

    int t=1;
    for (;pos<sv.size() && t>0;++pos) {
        if (sv[pos]==open) ++t;
        else if (sv[pos]==close) --t;
    }
    if (t==0) return pos;

    return std::string::npos;
}


/**
 * @brief Transforms an input string to lower case. Uses `std::tolower`.
 */
inline auto tolower(std::string s) {
    std::ranges::transform(s, s.begin(),
                           [](unsigned char c){ return std::tolower(c); });
    return s;
}
/**
 * @brief Transforms an input string to upper case. Uses `std::toupper`.
 */
inline auto toupper(std::string s) {
    std::ranges::transform(s, s.begin(),
                           [](unsigned char c){ return std::toupper(c); });
    return s;
}

/**
 * @brief From c++23: Reverses the bytes in the given integer value `n`. Call is ill-formed if `T` has padding bits.
 * 
 * @return an integer value of type `T` whose object representation comprises the bytes of that of `n` in reversed order.
 */
template <std::integral T>
constexpr T byteswap(T n) noexcept {
    static_assert(std::has_unique_object_representations_v<T>, 
                  "T may not have padding bits");
    auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(n);
    std::ranges::reverse(value_representation);
    return std::bit_cast<T>(value_representation);
}

}
