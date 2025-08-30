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
#include <utility>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>

#include <wt/math/format.hpp>

namespace wt {

/**
 * @brief Simple axis-aligned bounding box (AABB) primitive.
 */
struct aabb_t {
    pqvec3_t min;
    pqvec3_t max;

    aabb_t() noexcept = default;

    constexpr aabb_t(const pqvec3_t& min, const pqvec3_t& max) noexcept : min(min),max(max) {}
    explicit constexpr aabb_t(const pqvec3_t &v) : min(v), max(v) {}

    aabb_t(const aabb_t&) = default;
    aabb_t& operator=(const aabb_t&) = default;

    /**
     * @brief Union of two AABBs.
     */
    constexpr inline aabb_t operator|(const aabb_t& o) const noexcept {
        return aabb_t{ m::min(min,o.min), m::max(max,o.max) };
    }
    /**
     * @brief Union of two AABBs.
     */
    constexpr inline aabb_t& operator|=(const aabb_t& o) noexcept {
        *this = *this|o;
        return *this;
    }
    /**
     * @brief Union of two AABBs.
     */
    constexpr inline aabb_t operator|(const pqvec3_t& p) const noexcept {
        return aabb_t{ m::min(min,p), m::max(max,p) };
    }
    /**
     * @brief Union of two AABBs.
     */
    constexpr inline aabb_t& operator|=(const pqvec3_t& p) noexcept {
        *this = *this|p;
        return *this;
    }
    /**
     * @brief Intersection of two AABBs.
     */
    constexpr inline aabb_t operator&(const aabb_t& o) const noexcept {
        return aabb_t{ m::max(min,o.min), m::min(max,o.max) };
    }
    /**
     * @brief Intersection of two AABBs.
     */
    constexpr inline aabb_t& operator&=(const aabb_t& o) noexcept {
        *this = *this&o;
        return *this;
    }

    [[nodiscard]] constexpr inline bool operator==(const aabb_t&) const noexcept = default;

    [[nodiscard]] constexpr inline bool empty() const noexcept {
        return m::any(min>=max);
    }

    [[nodiscard]] constexpr inline bool isfinite() const noexcept {
        return m::isfinite(min) && m::isfinite(max);
    }

    /**
     * @brief Checks for overlaps between AABBs.
     */
    [[nodiscard]] constexpr inline bool overlaps(const aabb_t& o) const noexcept {
        return min.x<o.max.x && o.min.x<max.x &&
               min.y<o.max.y && o.min.y<max.y &&
               min.z<o.max.z && o.min.z<max.z;
    }

    /**
     * @brief Returns TRUE if p is inside the AABB. Template argument incl handles inclusivity on the AABB faces.
     */
    template <range_inclusiveness_e incl = range_inclusiveness_e::left_inclusive>
    [[nodiscard]] constexpr inline bool contains(const pqvec3_t& p) const noexcept {
        for (int i=0;i<3;++i)
            if (!pqrange_t<incl>{ min[i],max[i] }.contains(p[i]))
                return false;
        return true;
    }
    /**
     * @brief Returns TRUE if this AABB contains ``aabb``.
     */
    [[nodiscard]] constexpr inline bool contains(const aabb_t& aabb) const noexcept {
        return all(aabb.min>=min) &&
               all(aabb.max<=max);
    }

    /**
     * @brief Returns the closest point in the AABB, or on its faces, to the point ``p``.
     */
    [[nodiscard]] constexpr inline pqvec3_t closest_point(const pqvec3_t& p) const noexcept {
        return m::clamp(p, min,max);
    }

    /**
     * @brief Returns the distance squared from the AABB to the point ``p``.
     */
    [[nodiscard]] inline auto distance2(const pqvec3_t& p) const noexcept {
        const auto l = m::max(pqvec3_t::zero(), min-p, p-max);
        return m::length2(l);
    }

    /**
     * @brief Returns the distance from the AABB to the point ``p``.
     */
    [[nodiscard]] inline auto distance(const pqvec3_t& p) const noexcept {
        return m::sqrt(distance2(p));
    }

    [[nodiscard]] constexpr inline auto volume() const noexcept {
        const auto d = max-min;
        return d.x*d.y*d.z;
    }
    [[nodiscard]] constexpr inline auto surface_area() const noexcept {
        const auto l = max-min;
        return 2*(l.x*l.y + l.y*l.z + l.z*l.x);
    }
    /**
     * @brief Area of the AABB projected upon a plane with normal `dir'.
     */
    [[nodiscard]] constexpr inline auto surface_area(const dir3_t& dir) const noexcept {
        const auto l = max-min;
        const auto Ax = l.y*l.z;
        const auto Ay = l.x*l.z;
        const auto Az = l.x*l.y;
        return Ax * m::abs(dir.x) + Ay * m::abs(dir.y) + Az * m::abs(dir.z);
    }

    [[nodiscard]] constexpr inline auto centre() const noexcept {
        return (max+min)/f_t(2);
    }
    [[nodiscard]] constexpr inline auto extent() const noexcept {
        return max-min;
    }

    /**
     * @brief returns the axis of the AABB that is the longest
     * (0, 1, 2 for x, y, z respectively)
     */
    [[nodiscard]] constexpr inline int max_dimension() const noexcept {
        pqvec3_t d = extent();
        if (d.x > d.y && d.x > d.z) {
            return 0;
        } else if (d.y > d.z) {
            return 1;
        } else {
            return 2;
        }
    }

    [[nodiscard]] constexpr inline auto grow(const length_t extent) const noexcept {
        return clamp(aabb_t{ min-pqvec3_t{ extent }, max+pqvec3_t{ extent } });
    }
    [[nodiscard]] constexpr inline auto grow(const pqvec3_t& extent) const noexcept {
        return clamp(aabb_t{ min-extent,max+extent });
    }

    /**
     * @brief Splits the AABB along dimension ``axis`` at plane ``p`` (in world coordinates).
     */
    [[nodiscard]] inline std::pair<aabb_t,aabb_t> split(int axis,
                                                        length_t P) const noexcept {
        std::pair<aabb_t,aabb_t> S = { *this,*this };

        assert(P>=min[axis]);
        assert(P<=max[axis]);
        S.first.max[axis]  = P;
        S.second.min[axis] = P;

        return S;
    }


    /**
     * @brief Returns one of the 8 AABB vertices
     */
    [[nodiscard]] inline auto vertex(int vid) const noexcept {
        const auto x = !!(vid & 0x1);
        const auto y = !!(vid & 0x2);
        const auto z = !!(vid & 0x4);
        return m::mix(min,max, vec3b_t{ x,y,z });
    }

    [[nodiscard]] static inline auto face_normal(int face) noexcept {
        return std::array<dir3_t,6>{
            dir3_t{ 0,0,-1 },
            dir3_t{ 0,0,+1 },
            dir3_t{ 0,-1,0 },
            dir3_t{ 0,+1,0 },
            dir3_t{ -1,0,0 },
            dir3_t{ +1,0,0 },
        }[face];
    }


    [[nodiscard]] constexpr static inline aabb_t inf() noexcept {
        const length_t v = m::inf * isq::length[u::m];
        return aabb_t{
            pqvec3_t{ -v,-v,-v },
            pqvec3_t{ +v,+v,+v }
        };
    }
    [[nodiscard]] constexpr static inline aabb_t null() noexcept {
        const length_t v = m::inf * isq::length[u::m];
        return aabb_t{
            pqvec3_t{ +v,+v,+v },
            pqvec3_t{ -v,-v,-v }
        };
    }
    [[nodiscard]] constexpr static inline aabb_t clamp(const aabb_t& aabb) noexcept {
        return aabb_t{ aabb.min, m::max(aabb.max,aabb.min) };
    }

    [[nodiscard]] constexpr static inline aabb_t from_points(const std::convertible_to<pqvec3_t> auto& ...pts) {
        return ((aabb_t{ static_cast<pqvec3_t>(pts) }) | ...);
    }
};

}


template<>
struct std::formatter<wt::aabb_t> : std::formatter<wt::pqvec3_t> {
    auto format(const wt::aabb_t& aabb, std::format_context& ctx) const {
        const auto open_bracket  = "[";
        const auto close_bracket = "]";
        
        return std::format_to(ctx.out(), "{}{} .. {}{}",
                    open_bracket, aabb.min, aabb.max, close_bracket);
    }
};
