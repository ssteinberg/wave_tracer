/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/util/concepts.hpp>
#include <wt/math/common.hpp>
#include <wt/math/simd/wide_vector.hpp>

#include <cassert>

namespace wt {

/**
 * @brief Controls inclusiveness of end points of a ``range_t``.
 */
enum class range_inclusiveness_e : std::int8_t {
    /**
     * @brief Both ends included: ``[min,max]`` 
     */
    inclusive,
    /**
     * @brief Both ends excluded: ``(min,max)`` 
     */
    exclusive,
    /**
     * @brief Left end included: ``[min,max)`` 
     */
    left_inclusive,
    /**
     * @brief Right end included: ``(min,max]`` 
     */
    right_inclusive
};

/**
 * @brief Describes the one-dimensional range between ``min`` and ``max``.
 * @tparam inclusiveness dictates inclusiveness of endpoints.
 */
template <Scalar T=f_t, range_inclusiveness_e inclusiveness=range_inclusiveness_e::inclusive>
struct range_t {
    T min, max;

    [[nodiscard]] static constexpr inline bool includes_start_point() noexcept {
        return
            inclusiveness==range_inclusiveness_e::left_inclusive ||
            inclusiveness==range_inclusiveness_e::inclusive;
    }
    [[nodiscard]] static constexpr inline bool includes_end_point() noexcept {
        return
            inclusiveness==range_inclusiveness_e::right_inclusive ||
            inclusiveness==range_inclusiveness_e::inclusive;
    }

    /**
     * @brief Returns TRUE if the range contains the point.
     */
    [[nodiscard]] constexpr inline bool contains(T pt) const noexcept {
        if (pt<max && min<pt) return true;
        if constexpr (includes_start_point()) if (pt==min) return true;
        if constexpr (includes_end_point())   if (pt==max) return true;
        return false;
    }
    /**
     * @brief Returns TRUE if the range contains the point. Wide test.
     */
    template <std::size_t W>
    [[nodiscard]] constexpr inline b_w_t<W> contains(f_w_t<W> pt) const noexcept
        requires (std::is_same_v<T,f_t>)
    {
        b_w_t<W> cond1, cond2;
        if constexpr (includes_start_point()) cond1 = f_w_t<W>{ min } <= pt;
        else                                  cond1 = f_w_t<W>{ min } < pt;
        if constexpr (includes_end_point())   cond2 = f_w_t<W>{ max } >= pt;
        else                                  cond2 = f_w_t<W>{ max } > pt;

        return cond1 && cond2;
    }
    /**
     * @brief Returns TRUE if the range contains the point. Wide test.
     */
    template <std::size_t W>
    [[nodiscard]] constexpr inline b_w_t<W> contains(q_w_t<W,T> pt) const noexcept
        requires (is_quantity_v<T>)
    {
        b_w_t<W> cond1, cond2;
        if constexpr (includes_start_point()) cond1 = q_w_t<W,T>{ min } <= pt;
        else                                  cond1 = q_w_t<W,T>{ min } < pt;
        if constexpr (includes_end_point())   cond2 = q_w_t<W,T>{ max } >= pt;
        else                                  cond2 = q_w_t<W,T>{ max } > pt;

        return cond1 && cond2;
    }

    /**
     * @brief Returns TRUE if this range contains the input range ``range``.
     */
    [[nodiscard]] constexpr inline bool contains(range_t range) const noexcept {
        return min<=range.min && max>=range.max;
    }

    /**
     * @brief Checks for overlaps between ranges.
     */
    template <auto inc>
    [[nodiscard]] inline bool overlaps(const range_t<T,inc>& r) const noexcept {
        using R = range_t<T,inc>;
        bool check1, check2;
        if constexpr (includes_start_point() && R::includes_end_point()) check1 = min<=r.max;
        else                                                             check1 = min<r.max;
        if constexpr (R::includes_start_point() && includes_end_point()) check2 = r.min<=max;
        else                                                             check2 = r.min<max;
        return check1 && check2;
    }

    /**
     * @brief Union of two ranges.
     */
    template <auto inc>
    constexpr inline range_t operator|(const range_t<T,inc>& o) const noexcept {
        return range_t{ m::min(min,o.min), m::max(max,o.max) };
    }
    /**
     * @brief Union of two ranges.
     */
    template <auto inc>
    constexpr inline range_t& operator|=(const range_t<T,inc>& o) noexcept {
        *this = *this|o;
        return *this;
    }
    /**
     * @brief Intersection of two ranges.
     */
    template <auto inc>
    constexpr inline range_t operator&(const range_t<T,inc>& o) const noexcept {
        return range_t{ m::max(min,o.min), m::min(max,o.max) };
    }
    /**
     * @brief Intersection of two ranges.
     */
    template <auto inc>
    constexpr inline range_t& operator&=(const range_t<T,inc>& o) noexcept {
        *this = *this&o;
        return *this;
    }

    [[nodiscard]] constexpr inline bool operator==(const range_t& o) const noexcept {
        return (min==o.min && max==o.max) || (this->empty() && o.empty());
    }
    [[nodiscard]] constexpr inline bool operator!=(const range_t& o) const noexcept {
        return !(*this == o);
    }

    [[nodiscard]] constexpr inline bool empty() const noexcept {
        // empty when min==max==±∞, regardless for inclusivity
        if (min==max && !m::isfinite(min))
            return true;

        return inclusiveness==range_inclusiveness_e::inclusive ? min>max : min>=max;
    }

    /**
     * @brief Returns the length of the range.
     */
    [[nodiscard]] constexpr inline auto length() const noexcept {
        return max-min;
    }

    [[nodiscard]] constexpr inline auto centre() const noexcept {
        return (max+min)/f_t(2);
    }

    [[nodiscard]] constexpr inline auto grow(const T extent) const noexcept {
        return range_t{ min-extent, max+extent };
    }


    [[nodiscard]] inline auto& operator[](std::size_t idx) { assert(idx<=1); return idx==0 ? min : max; }
    [[nodiscard]] inline const auto& operator[](std::size_t idx) const { assert(idx<=1); return idx==0 ? min : max; }

    [[nodiscard]] constexpr inline auto size() const noexcept { return 2; }

    [[nodiscard]] inline auto begin() noexcept { return &min; }
    [[nodiscard]] inline auto end() noexcept { return &max+1; }
    [[nodiscard]] inline auto begin() const noexcept { return &min; }
    [[nodiscard]] inline auto end() const noexcept { return &max+1; }

    [[nodiscard]] inline auto& front() noexcept { return min; }
    [[nodiscard]] inline auto& back() noexcept { return max; }
    [[nodiscard]] inline const auto& front() const noexcept { return min; }
    [[nodiscard]] inline const auto& back() const noexcept { return max; }

    [[nodiscard]] inline auto cbegin() const noexcept { return &min; }
    [[nodiscard]] inline auto cend() const noexcept { return &max+1; }


    /**
     * @brief Constructs a range.
     */
    [[nodiscard]] constexpr static inline auto range(const T min, const T max) noexcept {
        return range_t{ .min=min, .max=max, };
    }
    /**
     * @brief Constructs a single point range (this range maybe empty, depending on ``inclusiveness``).
     */
    [[nodiscard]] constexpr static inline auto range(const T pt) noexcept {
        return range_t{ .min=pt, .max=pt, };
    }
    /**
     * @brief Constructs the range \f$ [0,+\infty) \f$.
     */
    [[nodiscard]] constexpr static inline auto positive() noexcept {
        if constexpr (limits<T>::has_infinity) {
            return range_t{
                .min = T{},
                .max = +limits<T>::infinity()
            };
        } else {
            return range_t{
                .min = T{},
                .max = limits<T>::max()
            };
        }
    }
    /**
     * @brief Constructs the range \f$ (-\infty,+\infty) \f$.
     */
    [[nodiscard]] constexpr static inline auto all() noexcept {
        if constexpr (limits<T>::has_infinity) {
            return range_t{
                .min = -limits<T>::infinity(),
                .max = +limits<T>::infinity()
            };
        } else {
            return range_t{
                .min = limits<T>::min(),
                .max = limits<T>::max()
            };
        }
    }
    /**
     * @brief Constructs an empty range.
     */
    [[nodiscard]] constexpr static inline auto null() noexcept {
        if constexpr (limits<T>::has_infinity) {
            return range_t{
                .min = +limits<T>::infinity(),
                .max = -limits<T>::infinity()
            };
        } else {
            return range_t{
                .min = limits<T>::max(),
                .max = limits<T>::min()
            };
        }
    }

    /**
     * @brief Comparison operator.
     *        This operator enables ``range_t`` to be used as a key in stl containers.
     */
    [[nodiscard]] bool operator<(const range_t& o) const noexcept {
        return min==o.min ? max<o.max : min<o.min;
    }
};

template <typename T>
constexpr inline auto operator*(const range_t<T>& range, f_t f) noexcept {
    return range_t<T>{ .min=range.min*f, .max=range.max*f };
}
template <typename T>
constexpr inline auto operator*(f_t f, const range_t<T>& range) noexcept {
    return range_t<T>{ .min=range.min*f, .max=range.max*f };
}
template <typename T>
constexpr inline auto operator/(const range_t<T>& range, f_t f) noexcept {
    return range_t<T>{ .min=range.min/f, .max=range.max/f };
}
template <typename T, typename S>
constexpr inline auto operator*(const range_t<T>& range, S s) noexcept {
    using R = decltype(std::declval<T>()*s);
    return range_t<R>{ .min=range.min*s, .max=range.max*s };
}
template <typename T, typename S>
constexpr inline auto operator*(S s, const range_t<T>& range) noexcept {
    using R = decltype(s*std::declval<T>());
    return range_t<R>{ .min=s*range.min, .max=s*range.max };
}
template <typename T, typename S>
constexpr inline auto operator/(const range_t<T>& range, S s) noexcept {
    using R = decltype(std::declval<T>()/s);
    return range_t<R>{ .min=range.min/s, .max=range.max/s };
}


template <range_inclusiveness_e inclusiveness=range_inclusiveness_e::inclusive>
using pqrange_t = range_t<length_t, inclusiveness>;


namespace m {

template <Scalar S, range_inclusiveness_e inclusiveness, NumericOrBool T>
constexpr inline auto mix(const range_t<S, inclusiveness>& r, 
                          const T& x) noexcept {
    if (x==T(0)) return r.min;
    if (x==T(1)) return r.max;
    return m::mix(r.min,r.max,x);
}

}

}


template <typename T, wt::range_inclusiveness_e inclusiveness>
struct std::formatter<wt::range_t<T, inclusiveness>> : std::formatter<T> {
    auto format(const wt::range_t<T, inclusiveness>& range, std::format_context& ctx) const {
        const auto open_bracket = 
            inclusiveness==wt::range_inclusiveness_e::inclusive || inclusiveness==wt::range_inclusiveness_e::left_inclusive ?
                "[" : "(";
        const auto close_bracket = 
            inclusiveness==wt::range_inclusiveness_e::inclusive || inclusiveness==wt::range_inclusiveness_e::right_inclusive ?
                "]" : ")";
        return std::format_to(ctx.out(), "{}{} .. {}{}",
                    open_bracket, range.min, range.max, close_bracket);
    }
}; 
