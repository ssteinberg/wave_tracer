/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/common.hpp>
#include <wt/math/simd/wide_vector.hpp>
#include <array>

namespace wt {

/**
 * @brief A *ball*: the interior of a spherical shell.
 */
struct ball_t {
    pqvec3_t centre;
    length_t radius;

    [[nodiscard]] constexpr inline bool empty() const noexcept {
        return radius==zero;
    }

    [[nodiscard]] constexpr inline bool isfinite() const noexcept {
        return m::isfinite(radius) && m::isfinite(centre);
    }

    [[nodiscard]] constexpr inline bool overlaps(const ball_t& o) const noexcept {
        return m::length2(centre-o.centre) < m::sqr(radius + o.radius);
    }

    /**
     * @brief Checks if the ball contains a point.
     */
    [[nodiscard]] constexpr inline bool contains(const pqvec3_t& p) const noexcept {
        return m::length2(centre-p) < m::sqr(radius);
    }
    /**
     * @brief Checks if the ball contains another ball.
     */
    [[nodiscard]] constexpr inline bool contains(const ball_t& ball) const noexcept {
        return m::length2(centre-ball.centre) < m::sqr(m::max<length_t>(0*u::m, radius-ball.radius));
    }

    /**
     * @brief Checks if the ball contains a point. Wide test.
     */
    template <std::size_t W>
    [[nodiscard]] inline auto contains(const pqvec3_w_t<W>& p) const noexcept {
        const auto r2 = q_w_t<W,area_t>{ m::sqr(radius) };
        const auto c  = pqvec3_w_t<W>{ centre };
        const auto cp = c - p;

        const auto cp2 = m::dot(cp, cp);
        return cp2 < r2;
    }

    /**
     * @brief Ball volume.
     */
    [[nodiscard]] constexpr inline auto volume() const noexcept {
        return f_t(4./3.) * m::pi * m::sqr(radius)*radius;
    }
    /**
     * @brief Surface area of the ball's spherical shell.
     */
    [[nodiscard]] constexpr inline auto surface_area() const noexcept {
        return 4 * m::pi * m::sqr(radius);
    }

    /**
     * @brief Constructs a ball with radius increased (or decreased) by `r`. Clamps radius to zero.
     */
    [[nodiscard]] constexpr inline auto grow(const length_t r) const noexcept {
        return ball_t{ .centre=centre, .radius=m::max<length_t>(0*u::m,radius+r) };
    }


    /**
     * @brief Constructs the minimal ball that *just* contains the input list of points (that is, at least one point will lie on the shell of the ball).
     */
    [[nodiscard]] constexpr static inline ball_t from_points(const std::convertible_to<pqvec3_t> auto& ...pts) {
        const pqvec3_t c = (pqvec3_t{ pts } + ...) / (f_t)sizeof...(pts);
        area_t r2 = {};
        for (const auto& p : std::array{ pqvec3_t{ pts }... })
            r2 = m::max<area_t>(r2,m::length2(p-c));
        
        return ball_t{ .centre=c, .radius=m::sqrt(r2) };
    }
};

}
