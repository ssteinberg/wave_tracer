/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <optional>
#include <string_view>
#include <wt/util/concepts.hpp>

// this also provides std::format for enums
#include <magic_enum/magic_enum_format.hpp>

namespace wt::format {

template <Enum E>
inline std::optional<E> parse_enum(const std::string_view& str) noexcept {
    return magic_enum::enum_cast<E>(str);
}

}
