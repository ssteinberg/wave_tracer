/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <utility>
#include <chrono>

namespace wt::format::chrono {

template <typename T>
struct is_duration {
    static constexpr bool value = false;
};
template <typename Rep, typename Period>
struct is_duration<std::chrono::duration<Rep, Period>> {
    static constexpr bool value = true;
};
template <typename Rep, typename Period>
struct is_duration<const std::chrono::duration<Rep, Period>> {
    static constexpr bool value = true;
};
template <typename Rep, typename Period>
struct is_duration<volatile std::chrono::duration<Rep, Period>> {
    static constexpr bool value = true;
};
template <typename Rep, typename Period>
struct is_duration<const volatile std::chrono::duration<Rep, Period>> {
    static constexpr bool value = true;
};
template <typename T>
constexpr inline auto is_duration_v = is_duration<T>::value;


/**
 * @brief Extracts Duration out of input, and returns extracted value and remainder.
 */
template <typename Duration, typename Rep, typename Period>
    requires (is_duration_v<Duration>)
constexpr inline auto extract_duration(const std::chrono::duration<Rep, Period>& duration) noexcept {
    const auto d = floor<Duration>(duration);
    return std::make_pair( d.count(), duration-d );
}

}
