/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/defs.hpp>
#include <wt/util/concepts.hpp>

namespace wt::m::eft {

/**
 * @brief Used for error reduced transformations.
 *        Adapted from PBR book.
 */
template <Scalar T>
struct compensated_fp_t {
    T val, err={};

    constexpr compensated_fp_t(T val) noexcept : val(val) {}
    constexpr compensated_fp_t(const compensated_fp_t&) noexcept = default;
    constexpr compensated_fp_t& operator=(const compensated_fp_t&) noexcept = default;
    constexpr compensated_fp_t& operator=(const T& f) noexcept { val=f; err=0; return *this; }

    template <Scalar S>
    constexpr auto& operator+=(S f) noexcept {
        const auto delta = f - err;
        const auto ns = val + delta;
        err = (ns - val) - delta;
        val = ns;
        return *this;
    }
    template <Scalar S>
    constexpr auto operator+(S f) noexcept {
        const auto delta = f - err;
        compensated_fp_t ret{ val + delta };
        ret.err = (ret.val - val) - delta;
        return ret;
    }

    constexpr explicit operator T() const noexcept { return val; }
};

}
