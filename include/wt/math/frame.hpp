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

namespace wt {


class frame_t {
public:
    dir3_t t,b,n;

    [[nodiscard]] inline pqvec2_t to_local(const pqvec2_t& v) const noexcept {
        return {
            m::dot(pqvec2_t{ v.x,v.y }, vec2_t{ t.x,t.y }),
            m::dot(pqvec2_t{ v.x,v.y }, vec2_t{ b.x,b.y })
        };
    }
    [[nodiscard]] inline pqvec3_t to_local(const pqvec3_t& v) const noexcept {
        return {
            m::dot(v, t),
            m::dot(v, b),
            m::dot(v, n)
        };
    }

    [[nodiscard]] inline pqvec3_t to_world(const pqvec2_t& v) const noexcept {
        return t*v.x + b*v.y;
    }
    [[nodiscard]] inline pqvec3_t to_world(const pqvec3_t& v) const noexcept {
        return t*v.x + b*v.y + n*v.z;
    }

    [[nodiscard]] inline vec2_t to_local(const vec2_t& v) const noexcept {
        return {
            m::dot(v, vec2_t{ t.x,t.y }),
            m::dot(v, vec2_t{ b.x,b.y })
        };
    }
    [[nodiscard]] inline vec3_t to_local(const vec3_t& v) const noexcept {
        return {
            m::dot(v, t),
            m::dot(v, b),
            m::dot(v, n)
        };
    }

    [[nodiscard]] inline auto to_world(const vec2_t& v) const noexcept {
        return t*v.x + b*v.y;
    }
    [[nodiscard]] inline auto to_world(const vec3_t& v) const noexcept {
        return t*v.x + b*v.y + n*v.z;
    }

    [[nodiscard]] inline dir3_t to_local(const dir3_t& v) const noexcept {
        return dir3_t{
            m::dot(v, t),
            m::dot(v, b),
            m::dot(v, n)
        };
    }

    [[nodiscard]] inline dir2_t to_local(const dir2_t& v) const noexcept {
        return dir2_t{
            m::dot(v, vec2_t{ t.x,t.y }),
            m::dot(v, vec2_t{ b.x,b.y })
        };
    }
    [[nodiscard]] inline dir3_t to_world(const dir3_t& v) const noexcept {
        return dir3_t{ t*v.x + b*v.y + n*v.z };
    }
    [[nodiscard]] inline dir3_t to_world(const dir2_t& v) const noexcept {
        return dir3_t{ t*v.x + b*v.y };
    }


    /**
     * @brief Tests the handness of the frame. Returns +1 for RH systems and -1 for LH systems.
     */
    [[nodiscard]] inline f_t handness() const noexcept {
        const auto h = m::dot(m::cross(n,t), b);
        assert(h!=0);
        return h>0 ? 1 : -1;
    }


    /**
     * @brief Returns a flipped frame (flipped t,b,n)
     */
    [[nodiscard]] inline auto flip() const noexcept {
        return frame_t{
            .t = -t,
            .b = -b,
            .n = -n,
        };
    }

    /**
     * @brief Returns a flipped frame (flipped t,b,n)
     */
    [[nodiscard]] inline auto operator-() const noexcept {
        return flip();
    }

    /**
     * @brief Returns a frame with flipped handness (flipped bitangent)
     */
    [[nodiscard]] inline auto flip_handness() const noexcept {
        return frame_t{
            .t = t,
            .b = -b,
            .n = n,
        };
    }


    /**
     * @brief Returns the canonical frame: normal direction aligns with z axis, and tangent and bitangent with x and y axes, respectively.
     */
    [[nodiscard]] static inline frame_t canonical() noexcept {
        return frame_t{
            .t = dir3_t{ 1,0,0 },
            .b = dir3_t{ 0,1,0 },
            .n = dir3_t{ 0,0,1 },
        };
    }

    /**
     * @brief Builds a frame with normal ``n`` and its tangent direction aligning as closely as possible with ``dpdu``.
     */
    [[nodiscard]] static inline frame_t build_shading_frame(const dir3_t& n, 
                                                            const pqvec3_t& dpdu) noexcept {
        if (m::all(m::iszero(dpdu)))
            return build_orthogonal_frame(n);

        const auto t = m::normalize(dpdu - n*m::dot(n,dpdu));
        const auto b = m::normalize(m::cross(n, t));
        assert_iszero(m::dot(b,n));

        return frame_t{
            .t = dir3_t{ m::cross(b, n) },
            .b = dir3_t{ b },
            .n = n,
        };
    }

    /**
     * @brief Builds an arbitrary frame with normal ``n``.
     */
    [[nodiscard]] static inline frame_t build_orthogonal_frame(const dir3_t& n) noexcept {
        vec3_t b;
        if (m::abs(n.x)>m::abs(n.y)) {
            const f_t x = 1/m::sqrt(m::sqr(n.x)+m::sqr(n.z));
            b = vec3_t{ x*n.z, 0, -x*n.x };
        }
        else {
            const f_t x = 1/m::sqrt(m::sqr(n.y)+m::sqr(n.z));
            b = vec3_t{ 0, x*n.z, -x*n.y };
        }

        return frame_t{
            .t = dir3_t{ m::cross(b, n) },
            .b = dir3_t{ b },
            .n = n,
        };
    }


    /**
     * @brief Transforms the frame via ``R``. ``R`` is assumed to be an orthogonal matrix.
     */
    [[nodiscard]] friend inline auto operator*(const mat3_t& R, const frame_t& f) noexcept {
        assert_iszero(1-m::abs(m::determinant(R)));
        return frame_t{
            .t = dir3_t{ R * f.t },
            .b = dir3_t{ R * f.b },
            .n = dir3_t{ R * f.n },
        };
    }

    /**
     * @brief Vectorized 8x transform to local space. Input in metres.
     */
    template <std::size_t W>
    [[nodiscard]] inline auto to_local(const pqvec3_w_t<W>& v) const noexcept {
        return pqvec3_w_t<W>{
            m::dot(v, vec3_w_t<W>{ vec3_t{ t } }),
            m::dot(v, vec3_w_t<W>{ vec3_t{ b } }),
            m::dot(v, vec3_w_t<W>{ vec3_t{ n } })
        };
    }

    /**
     * @brief Vectorized 8x transform to local space. Dimensionless input.
     */
    template <std::size_t W>
    [[nodiscard]] inline auto to_local(const vec3_w_t<W>& v) const noexcept {
        return vec3_w_t<W>{
            m::dot(v, vec3_w_t<W>{ vec3_t{ t } }),
            m::dot(v, vec3_w_t<W>{ vec3_t{ b } }),
            m::dot(v, vec3_w_t<W>{ vec3_t{ n } })
        };
    }
};

}
