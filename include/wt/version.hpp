/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cstdint>
#include <string>

namespace wt {

struct wt_version_t {
private:
    std::string build_str;
    std::uint16_t mjr{},mnr{},ptch{};

public:
    wt_version_t();
    wt_version_t(const wt_version_t&) noexcept = default;
    wt_version_t& operator=(const wt_version_t&) noexcept = default;

    [[nodiscard]] inline auto major() const noexcept { return mjr; }
    [[nodiscard]] inline auto minor() const noexcept { return mnr; }
    [[nodiscard]] inline auto patch() const noexcept { return ptch; }
    [[nodiscard]] inline const auto& build_string() const noexcept { return build_str; }

    void print_wt_version();
    std::string short_version_string();
    std::string full_version_string();
};

}
