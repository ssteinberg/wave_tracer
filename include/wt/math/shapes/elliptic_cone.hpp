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
#include <wt/math/frame.hpp>
#include <wt/math/range.hpp>
#include <wt/math/shapes/ray.hpp>
#include <wt/math/simd/wide_vector.hpp>

#include <wt/interaction/intersection.hpp>

#include <cassert>
#include <wt/util/assert.hpp>


namespace wt {

/**
 * @brief Quantifies the geometry of an elliptical cone.
 *        Supports the degenerate cases, where the cone degenerates into an (infinite) cylinder or a ray.
 */
class elliptic_cone_t {
private:
    ray_t r;
    dir3_t tangent;
    length_t initial_x_length = 0 * u::m;

    f_t one_over_e;
    f_t e;

    f_t tan_alpha;

    length_t z_apex;

public:
    /**
     * @brief Construct a new elliptic cone object
     *
     * @param r central ray
     * @param tan_alpha tan of half opening angle
     * @param initial_x initial major axis length
     */
    explicit elliptic_cone_t(const ray_t& r, f_t tan_alpha, length_t initial_x=0*u::m)
        : elliptic_cone_t(r, frame_t::build_orthogonal_frame(r.d).t,
                          tan_alpha, 0, initial_x)
    {}
    /**
     * @brief Construct a new elliptic cone object
     *
     * @param r central ray
     * @param x direction of major axis (must be tangent to central ray direction)
     * @param tan_alpha tan of half opening angle
     * @param eccentricity ∈[0,1) elliptical cone eccentricity (0 - isotropic, 1 - degenerate flat cone)
     * @param initial_x initial major axis length
     */
    explicit elliptic_cone_t(const ray_t& r, const dir3_t& x,
                             f_t tan_alpha, f_t eccentricity, length_t initial_x=0*u::m)
        : r(r), tangent(x),
          initial_x_length(initial_x),
          one_over_e(m::sqrt(m::max<f_t>(0,1-m::sqr(eccentricity)))),
          e(f_t(1)/one_over_e),
          tan_alpha(tan_alpha)
    {
        z_apex = initial_x_length!=zero || tan_alpha!=zero ?
                    -initial_x_length / tan_alpha :
                    -limits<length_t>::infinity();      // degenerate ray

        assert(tan_alpha>=zero && initial_x_length>=zero);
        assert(1>eccentricity && eccentricity>=0 && e>=1);
        assert_iszero(m::dot(r.d,x));
    }
    elliptic_cone_t(const elliptic_cone_t&) = default;
    elliptic_cone_t& operator=(const elliptic_cone_t&) = default;

    /**
     * @brief Returns true if the elliptic cone is degenerate: eccentricity=1
     */
    [[nodiscard]] inline bool degenerate() const noexcept {
        return tan_alpha==0 || one_over_e==0;
    }
    /**
     * @brief Returns true if the elliptic cone is a ray: \f$ \alpha=0 \land x0==0 \f$
     */
    [[nodiscard]] inline bool is_ray() const noexcept {
        return tan_alpha==0 && initial_x_length==zero;
    }
    /**
     * @brief Returns true if the elliptic cone is an elliptical frustum
     */
    [[nodiscard]] inline bool is_elliptical_frustum() const noexcept {
        return tan_alpha==0;
    }

    /** @brief Centre ray.
     */
    [[nodiscard]] const auto& ray() const noexcept { return r; }
    /** @brief Origin.
     */
    [[nodiscard]] const auto& o() const noexcept { return r.o; }
    /** @brief Mean direction of propagation (local z axis).
     */
    [[nodiscard]] const auto& d() const noexcept { return r.d; }
    /** @brief Tangent direction (local x axis).
     */
    [[nodiscard]] const auto& x() const noexcept { return tangent; }
    /** @brief Bi-tangent direction (local y axis).
     */
    [[nodiscard]] const auto y() const noexcept { return dir3_t{ m::cross(r.d,x()) }; }

    /** @brief The initial major axis length (in tangent direction) at origin.
     */
    [[nodiscard]] constexpr auto x0() const { return initial_x_length; }
    /** @brief Tan of half opening angle.
     */
    [[nodiscard]] constexpr auto get_tan_alpha() const { return tan_alpha; }
    /** @brief \f$ \frac{1}{\sqrt{1-\epsilon^2}} \equiv \frac{\textrm{major}}{\textrm{minor}} \f$, where \f$ \epsilon \f$ is eccentricity. Can be \f$ +\infty \f$.
     */
    [[nodiscard]] constexpr auto get_e() const { return e; }
    /** @brief \f$ \sqrt{1-\epsilon^2} \equiv \frac{\textrm{minor}}{\textrm{major}} \f$, where \f$ \epsilon \f$ is eccentricity.
     */
    [[nodiscard]] constexpr auto get_one_over_e() const { return one_over_e; }
    /** @brief Precomputed z position of apex point, can be \f$ -\infty \f$.
     */
    [[nodiscard]] constexpr auto get_z_apex() const { return z_apex; }

    /** @brief Translate origin by an offset.
     */
    void offset(const pqvec3_t& offset) noexcept {
        r.o += offset;
    }
    void set_o(const pqvec3_t& newo) noexcept { r.o = newo; }
    void set_x0(const length_t& newx0) noexcept {
        initial_x_length = newx0;
        z_apex = initial_x_length!=zero || tan_alpha!=zero ?
                    -initial_x_length / tan_alpha :
                    -limits<length_t>::infinity();      // degenerate ray
    }

    /** @brief Returns local frame.
     */
    [[nodiscard]] inline frame_t frame() const noexcept {
        return { x(),y(),d() };
    }

    /**
     * @brief Returns true if the elliptic cone contains the point
     *
     * @param range restrict z distance to this range
     */
    [[nodiscard]] inline bool contains(const pqvec3_t& p, 
                                       const pqrange_t<>& range = { 0 * u::m, +m::inf * u::m }) const noexcept {
        return contains_local(frame().to_local(p-r.o), range);
    }
    /**
     * @brief Returns true if the elliptic cone contains the point 
     *        (local coordinates)
     *
     * @param range restrict z distance to this range
     */
    [[nodiscard]] inline bool contains_local(const pqvec3_t& p, 
                                             const pqrange_t<>& range = { 0 * u::m, +m::inf * u::m }) const noexcept {
        return range.contains(p.z) && z_apex<=p.z && 
               m::sqr(p.x) + m::sqr(e*p.y) <= m::sqr(p.z*tan_alpha + initial_x_length);
    }

    /**
     * @brief Wide contains_local(). Input assumed to be in metres.
     *
     * @param range restrict z distance to this range
     */
    template <std::size_t W>
    [[nodiscard]] inline auto contains_local(const pqvec3_w_t<W>& p,
                                             const pqrange_t<>& range) const noexcept {
        const auto x2  = m::sqr(p.x());
        const auto ey  = p.y() * f_w_t<W>{ e };
        const auto ztx = m::fma(p.z(), f_w_t<W>(tan_alpha), length_w_t<W>(initial_x_length));

        const auto ey2  = m::sqr(ey);
        const auto ztx2 = m::sqr(ztx);

        const auto cond1 = length_w_t<W>{ z_apex } <= p.z();
        const auto cond2 = range.contains(p.z());
        const auto cond3 = (x2+ey2) <= ztx2;

        return cond1 && cond2 && cond3;
    }
    /**
     * @brief Wide contains_local(). Input assumed to be in metres.
     */
    template <std::size_t W>
    [[nodiscard]] inline auto contains_local(const pqvec3_w_t<W>& p) const noexcept {
        const auto x2  = m::sqr(p.x());
        const auto ey  = p.y() * f_w_t<W>{ e };
        const auto ztx = m::fma(p.z(), f_w_t<W>(tan_alpha), length_w_t<W>(initial_x_length));

        const auto ey2  = m::sqr(ey);
        const auto ztx2 = m::sqr(ztx);

        const auto cond1 = length_w_t<W>{ z_apex } <= p.z();
        const auto cond2 = (x2+ey2) <= ztx2;

        return cond1 && cond2;
    }

    /**
     * @brief Returns the point p after projection onto the elliptic cone cross section after propagation of a distance of z. The point 'p' is given in local frame.
     */
    [[nodiscard]] inline pqvec2_t project_local(const pqvec3_t& p, const length_t z) const noexcept {
        const auto xy = pqvec2_t{ p };

        const auto z0 = p.z;
        const auto scale = (tan_alpha*z + initial_x_length) / m::abs(tan_alpha*z0 + initial_x_length);
        
        return initial_x_length==zero && tan_alpha==zero ? xy : 
            xy * scale;
    }
    /**
     * @brief Returns the point p after projection onto the elliptic cone cross section after propagation of a distance of z.
     */
    [[nodiscard]] inline pqvec2_t project(const pqvec3_t& p, const length_t z) const noexcept {
        const auto u  = frame().to_local(p-r.o);
        return project_local(u,z);
    }

    /**
     * @brief Returns the radius in local direction of the elliptic cone cross-section after propagation a distance of z.
     *        (r=(1,0) and r=(0,1) return the lengths of major and minor axes, respectively)
     * @param z distance of propagation
     * @param r direction on cross section
     * @return radius
     */
    [[nodiscard]] inline length_t radius(const length_t z, const dir2_t& r) const noexcept {
        const auto& axes = this->axes(z);
        const auto& a  = axes.x, b = axes.y;
        const auto cos2 = m::sqr(m::dot(r,dir2_t{ 1,0 }));

        return a==zero || b==zero ? 0*u::m :
                a*b / m::sqrt(m::sqr(a)*(1-cos2) + m::sqr(b)*cos2);
    }

    /**
     * @brief Returns the major and minor axes (x and y), in local frame, of the elliptic cone cross-section after propagation a distance of z.
     * @param z distance of propagation
     * @return axes length
     */
    [[nodiscard]] inline pqvec2_t axes(const length_t z) const noexcept {
        const auto r = tan_alpha*z + initial_x_length;
        return r*vec2_t{ 1,one_over_e };
    }

    /**
     * @brief Computes an elliptical cone with a fixed propagation direction that passes through an ellipse centred in the origin.
     * @param x ellipse major axis (direction and length)
     * @param y ellipse minor axis (direction and length, assumed to be orthogonal to x)
     * @param n normal of ellipse
     * @param ray desired cone central ray
     * @param tan_alpha desired tan of cone alpha
     * @param self_intersection_distance (out) the distance past the origin over which the cone continues to intersect the ellipse
     * @return cone in the ellipse local frame
     */
    static elliptic_cone_t cone_through_ellipse(const pqvec3_t& x, const pqvec3_t& y, const dir3_t& n, 
                                                const ray_t& ray, 
                                                const f_t tan_alpha, 
                                                length_t* self_intersection_distance=nullptr) noexcept;
    /**
     * @brief Computes an elliptical cone with a fixed propagation direction that passes through an ellipse centred in the origin.
     * @param surface surface intersection record
     * @param ray desired cone central ray
     * @param tan_alpha desired tan of cone alpha
     * @param self_intersection_distance (out) the distance past the origin over which the cone continues to intersect the ellipse
     * @return cone in the ellipse local frame
     */
    static elliptic_cone_t cone_through_ellipse(const intersection_surface_t& surface,
                                                const ray_t& ray,
                                                const f_t tan_alpha, 
                                                length_t* self_intersection_distance=nullptr) noexcept {
        const auto& a = surface.footprint.a();
        const auto& b = surface.footprint.b();

        assert(m::length2(a) * m::length2(b) >= zero);

        const auto& n = surface.geo.n;
        const auto& wa = surface.geo.to_world(a);
        const auto& wb = surface.geo.to_world(b);

        assert_iszero<f_t>(u::to_m(m::eft::dot(wa,n)), 10);
        assert_iszero<f_t>(u::to_m(m::eft::dot(wb,n)), 10);

        return cone_through_ellipse(wa, wb,
                                    n, ray, tan_alpha, 
                                    self_intersection_distance);
    }

    /**
     * @brief Computes an elliptical cone with a fixed propagation direction that passes through an ellipsoid centred in the origin.
     * @param axes ellipsoid axes lengths
     * @param axes_frame ellipsoid axes world frame
     * @param ray desired cone central ray
     * @param tan_alpha desired tan of cone alpha
     * @return cone in the ellipsoids local frame
     */
    static elliptic_cone_t cone_through_ellipsoid(const pqvec3_t& axes, 
                                                  const frame_t& axes_frame,
                                                  const ray_t& ray, 
                                                  const f_t tan_alpha) noexcept;

private:
    explicit elliptic_cone_t(const ray_t& r, const dir3_t& x,
           length_t initial_x_length,
           f_t tan_alpha,
           f_t one_over_e, f_t e) 
        : r(r), tangent(x), 
          initial_x_length(initial_x_length),
          one_over_e(one_over_e), e(e),
          tan_alpha(tan_alpha)
    {
        z_apex = initial_x_length!=zero || tan_alpha!=zero ?
                    -initial_x_length / tan_alpha :
                    -limits<length_t>::infinity();      // degenerate ray

        assert(tan_alpha>=zero && initial_x_length>=zero);
        assert(e>=1 && one_over_e>=0);
        assert_iszero(m::dot(r.d,x));
    }
};

}
