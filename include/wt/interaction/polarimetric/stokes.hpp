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

#include <wt/math/common.hpp>
#include <wt/math/frame.hpp>
#include <wt/math/rotation.hpp>

namespace wt {

/**
 * @brief Stokes parameters vector.
 * @tparam Q quantity of vector elements
 */
template <Quantity Q>
struct stokes_parameters_t {
    using quantity_type = Q;

    qvec4<Q> S{};

    [[nodiscard]] inline bool isfinite() const noexcept {
        return m::isfinite(S);
    }
    [[nodiscard]] inline bool isnan() const noexcept {
        return m::isnan(S);
    }

    /**
     * @brief Returns the polarization state vector.
     */
    [[nodiscard]] inline auto polarization_state() const noexcept {
        return qvec3<Q>{ S[1],S[2],S[3] };
    }

    /**
     * @brief Returns TRUE for perfectly randomly polarized Stokes parameters vector.
     */
    [[nodiscard]] inline auto is_unpolarized() const noexcept {
        return polarization_state() == qvec3<Q>{};
    }

    /**
     * @brief 0-th element of the Stokes parameters, i.e. total intensity.
     *        "Intensity" is used in a generalized sense, the actual units depend on Q.
     */
    [[nodiscard]] inline auto intensity() const noexcept {
        return S[0];
    }

    /**
     * @brief Intensity of polarized light.
     *        "Intensity" is used in a generalized sense, the actual units depend on Q.
     */
    [[nodiscard]] inline auto polarized_intensity() const noexcept {
        return m::length(polarization_state());
    }

    /**
     * @brief Intensity of randomly polarized light.
     *        "Intensity" is used in a generalized sense, the actual units depend on Q.
     */
    [[nodiscard]] inline auto unpolarized_intensity() const noexcept {
        return m::max(Q{}, intensity() - polarized_intensity());
    }

    /**
     * @brief Intensity of linearly-polarized light.
     *        "Intensity" is used in a generalized sense, the actual units depend on Q.
     */
    [[nodiscard]] inline auto linearly_polarized_intensity() const noexcept {
        const auto& Sp = polarization_state();
        return m::length(qvec2<Q>{ Sp.x,Sp.y });
    }

    /**
     * @brief Intensity of circularly-polarized light.
     *        "Intensity" is used in a generalized sense, the actual units depend on Q.
     */
    [[nodiscard]] inline auto circularly_polarized_intensity() const noexcept {
        return m::abs(polarization_state().z);
    }

    /**
     * @brief Degree of polarization.
     */
    [[nodiscard]] inline f_t degree_of_polarization() const noexcept {
        const auto I = intensity();
        return I>zero ? (f_t)(polarized_intensity()/I) : 0;
    }

    /**
     * @brief Degree of linear polarization.
     */
    [[nodiscard]] inline auto degree_of_linear_polarization() const noexcept {
        const auto I = intensity();
        return I>zero ? (f_t)(linearly_polarized_intensity()/I) : 0;
    }

    /**
     * @brief Degree of circular polarization.
     */
    [[nodiscard]] inline auto degree_of_circular_polarization() const noexcept {
        const auto I = intensity();
        return I>zero ? (f_t)(circularly_polarized_intensity()/I) : 0;
    }

    /**
     * @brief Angle of the linearly-polarized part.
     */
    [[nodiscard]] inline Angle auto linear_polarization_angle() const noexcept {
        const auto Sp = u::to_num(polarization_state() / intensity());
        return f_t(.5) * m::atan2(Sp.x,Sp.y);
    }

    /**
     * @brief Is circularly-polarized part right-hand polarized?
     */
    [[nodiscard]] inline bool is_circularly_polarized_rhs() const noexcept {
        const auto& Sp = polarization_state();
        return Sp.z>zero;
    }

    /**
     * @brief Returns the Stokes parameters vector with its frame handness flipped.
     */
    [[nodiscard]] auto flip_handness() const noexcept {
        return stokes_parameters_t{
            .S = { S[0], S[1], -S[2], -S[3] },
        };
    }

    /**
     * @brief Returns the Stokes parameters vector reoriented to align with the new tangent direction.
     * @param current_frame current frame orientation
     * @param new_frame desired frame orientation
     */
    [[nodiscard]] auto reorient(const frame_t& current_frame,
                                const frame_t& new_frame) const noexcept {
        assert_iszero(1-m::abs(m::dot(current_frame.n, new_frame.n)));

        // align tangent direction
        const auto tou = dir2_t{ vec2_t{ current_frame.to_local(new_frame.t) } };
        const auto tov = dir2_t{ vec2_t{ current_frame.to_local(new_frame.b) } };

        const auto R = util::rotation_matrix(dir2_t{ 1,0 }, tou);
        const auto S12 = R * (R * qvec2<Q>{ this->S[1], this->S[2] });
        const auto S = stokes_parameters_t<Q>{ .S = { this->S[0], S12.x, S12.y, this->S[3] } };

        [[maybe_unused]] const auto u = R * dir2_t{ 1,0 };
        assert_iszero<f_t>(1-m::dot(u,tou), 10);
        // handness change?
        const auto v = R * dir2_t{ 0,1 };
        if (m::dot(v,tov) < 0)
            return S.flip_handness();
        return S;
    }


    template <Quantity Q2>
        requires std::is_convertible_v<Q, Q2>
    [[nodiscard]] inline explicit operator stokes_parameters_t<Q2>() const noexcept {
        return stokes_parameters_t<Q2>{
            .S = static_cast<qvec4<Q2>>(S),
        };
    }

    [[nodiscard]] inline bool operator==(const stokes_parameters_t&) const noexcept = default;

    inline auto& operator*=(f_t scale) noexcept {
        S *= scale;
        return *this;
    }
    inline auto& operator/=(f_t scale) noexcept {
        S /= scale;
        return *this;
    }

    inline auto& operator+=(const stokes_parameters_t& S2) noexcept {
        S += S2.S;
        return *this;
    }
    inline auto operator+(const stokes_parameters_t& S2) const noexcept {
        return stokes_parameters_t{
            .S = S + S2.S,
        };
    }

    static auto zero() noexcept {
        return stokes_parameters_t{
            .S = { Q::zero(),Q::zero(),Q::zero(),Q::zero() },
        };
    }
    static auto unpolarized(Q I) noexcept {
        return stokes_parameters_t{
            .S = { I,Q::zero(),Q::zero(),Q::zero() },
        };
    }
    static auto linearly_polarized(Angle auto lp_angle, Q I) noexcept {
        return stokes_parameters_t{
            .S = { I, I*m::cos(2*lp_angle), I*m::sin(2*lp_angle), Q::zero() },
        };
    }
    static auto linearly_polarized_0deg(Q I) noexcept {
        return stokes_parameters_t{
            .S = { I,I,Q::zero(),Q::zero() },
        };
    }
    static auto linearly_polarized_45deg(Q I) noexcept {
        return stokes_parameters_t{
            .S = { I,Q::zero(),I,Q::zero() },
        };
    }
    static auto linearly_polarized_90deg(Q I) noexcept {
        return stokes_parameters_t{
            .S = { I,-I,Q::zero(),Q::zero() },
        };
    }
    static auto linearly_polarized_135deg(Q I) noexcept {
        return stokes_parameters_t{
            .S = { I,Q::zero(),-I,Q::zero() },
        };
    }
    static auto circularly_polarized(bool rhc, Q I) noexcept {
        return stokes_parameters_t{
            .S = { I,Q::zero(),Q::zero(),I*f_t(rhc?+1:-1) },
        };
    }
};

template <Quantity Q>
inline bool operator==(const stokes_parameters_t<Q>& s, zero_t) noexcept {
    return s.intensity() == Q::zero();
}
template <Quantity Q>
inline bool operator==(zero_t, const stokes_parameters_t<Q>& s) noexcept {
    return Q::zero() == s.intensity();
}
template <Quantity Q>
inline bool operator!=(const stokes_parameters_t<Q>& s, zero_t) noexcept {
    return s.intensity() != Q::zero();
}
template <Quantity Q>
inline bool operator!=(zero_t, const stokes_parameters_t<Q>& s) noexcept {
    return Q::zero() != s.intensity();
}


template <Quantity Q, ScalarOrUnit T>
[[nodiscard]] inline auto operator*(const stokes_parameters_t<Q>& S, const T f) noexcept {
    using RQ = decltype(std::declval<Q>()*std::declval<T>());
    return stokes_parameters_t<RQ>{
        .S = S.S*f,
    };
}
template <Quantity Q, ScalarOrUnit T>
[[nodiscard]] inline auto operator*(const T f, const stokes_parameters_t<Q>& S) noexcept {
    using RQ = decltype(std::declval<T>()*std::declval<Q>());
    return stokes_parameters_t<RQ>{
        .S = f*S.S,
    };
}
template <Quantity Q, ScalarOrUnit T>
[[nodiscard]] inline auto operator/(const stokes_parameters_t<Q>& S, const T f) noexcept {
    using RQ = decltype(std::declval<Q>()/std::declval<T>());
    return stokes_parameters_t<RQ>{
        .S = S.S/f,
    };
}


// importance (quantum efficiency) Stokes parameters vector
using QE_stokes_t = stokes_parameters_t<QE_t>;
// diffuse importance (i.e. QE × flux per area) Stokes parameters vector
using QE_solid_angle_stokes_t = stokes_parameters_t<QE_solid_angle_t>;
// importance intensity (i.e. QE × solid angle) Stokes parameters vector
using QE_area_stokes_t = stokes_parameters_t<QE_area_t>;
// importance flux Stokes parameters vector
using QE_flux_stokes_t = stokes_parameters_t<QE_flux_t>;

// radiant flux Stokes parameters vector
using radiant_flux_stokes_t = stokes_parameters_t<radiant_flux_t>;
// irradiance Stokes parameters vector
using irradiance_stokes_t = stokes_parameters_t<irradiance_t>;
// radiant intensity Stokes parameters vector
using radiant_intensity_stokes_t = stokes_parameters_t<radiant_intensity_t>;
// radiance Stokes parameters vector
using radiance_stokes_t = stokes_parameters_t<radiance_t>;

// spectral radiant flux Stokes parameters vector
using spectral_radiant_flux_stokes_t = stokes_parameters_t<spectral_radiant_flux_t>;
// spectral irradiance Stokes parameters vector
using spectral_irradiance_stokes_t = stokes_parameters_t<spectral_irradiance_t>;
// spectral radiant intensity Stokes parameters vector
using spectral_radiant_intensity_stokes_t = stokes_parameters_t<spectral_radiant_intensity_t>;
// spectral radiance Stokes parameters vector
using spectral_radiance_stokes_t = stokes_parameters_t<spectral_radiance_t>;

}


template <wt::Quantity Q>
struct std::formatter<wt::stokes_parameters_t<Q>> : std::formatter<wt::qvec4<Q>> {
    auto format(const wt::stokes_parameters_t<Q>& S, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", S.S);
    }
};
