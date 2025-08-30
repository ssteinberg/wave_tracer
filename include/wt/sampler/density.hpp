/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cassert>
#include <type_traits>

#include <wt/math/common.hpp>
#include <wt/sampler/measure.hpp>
#include <wt/math/quantity/defs.hpp>

namespace wt {

/**
 * @brief Simple abstraction for the common case where we sample/query a density that might be either continuos or discrete.
 */
template <Quantity DensityQ>
    requires std::is_same_v<typename DensityQ::rep, f_t>
struct sampling_pd_t {
    constexpr sampling_pd_t() noexcept = default;
    constexpr sampling_pd_t(DensityQ d) noexcept : d(d) { assert(d>=zero); }

    /**
     * @brief Constructs a sample with discrete probability mass `mass`.
     */
    static inline auto discrete(f_t mass) noexcept {
        sampling_pd_t pd;
        pd.m = -mass;
        assert(mass>=0);

        return pd;
    }

    sampling_pd_t(const sampling_pd_t&) noexcept = default;
    sampling_pd_t& operator=(const sampling_pd_t&) noexcept = default;

    /**
     * @brief Measure of the represented density.
     */
    [[nodiscard]] inline measure_e measure() const noexcept {
        return std::signbit(m) ? measure_e::discrete : measure_e::continuos;
    }
    [[nodiscard]] inline auto is_discrete() const noexcept { return measure() == measure_e::discrete; }

    /**
     * @brief Returns the density. `measure()` must be `wt::measure_e::continuos`.
     */
    [[nodiscard]] inline auto density() const noexcept {
        assert(!is_discrete());
        return d;
    }
    /**
     * @brief Returns the density for continuos samples, otherwise returns 0.
     */
    [[nodiscard]] inline auto density_or_zero() const noexcept {
        return is_discrete() ? DensityQ{} : density();
    }
    /**
     * @brief Returns the probability mass. `measure()` must be `wt::measure_e::discrete`.
     */
    [[nodiscard]] inline auto mass() const noexcept {
        assert(is_discrete());
        return -m;
    }

    [[nodiscard]] inline bool isfinite() const noexcept { return m::isfinite(m); }
    [[nodiscard]] inline bool isnan() const noexcept { return m::isnan(m); }

    inline friend sampling_pd_t operator*(f_t f, sampling_pd_t s) noexcept {
        assert(f>=0);
        s.m = f * s.m;
        return s;
    }
    inline friend sampling_pd_t operator*(sampling_pd_t s, f_t f) noexcept {
        assert(f>=0);
        s.m *= f;
        return s;
    }
    inline friend sampling_pd_t operator/(sampling_pd_t s, f_t f) noexcept {
        assert(f>0);
        s.m /= f;
        return s;
    }

    inline auto& operator*=(f_t f) noexcept {
        assert(f>=0);
        m *= f;
        return *this;
    }
    inline auto& operator/=(f_t f) noexcept {
        assert(f>0);
        m /= f;
        return *this;
    }

    inline friend bool operator==(const sampling_pd_t& s, zero_t) noexcept {
        return s.m==0;
    }
    inline friend bool operator==(zero_t, const sampling_pd_t& s) noexcept {
        return s.m==0;
    }
    inline friend bool operator!=(const sampling_pd_t& s, zero_t) noexcept {
        return s.m!=0;
    }
    inline friend bool operator!=(zero_t, const sampling_pd_t& s) noexcept {
        return s.m!=0;
    }

private:
    union {
        DensityQ d;
        f_t m = 0;
    };
};

using area_sampling_pd_t = sampling_pd_t<area_density_t>;
using angle_sampling_pd_t = sampling_pd_t<angle_density_t>;
using solid_angle_sampling_pd_t = sampling_pd_t<solid_angle_density_t>;

using wavenumber_sampling_pd_t = sampling_pd_t<wavenumber_density_t>;
using wavelength_sampling_pd_t = sampling_pd_t<wavelength_density_t>;

}
