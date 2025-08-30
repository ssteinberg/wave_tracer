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

#include <cmath>
#include <array>

namespace wt::m {

namespace detail {

struct erf_lut_t {
	static constexpr std::size_t N = 1024;
	static constexpr f_t maxX = 3.5;
    static constexpr auto scale = f_t(N-1)/maxX;

	std::array<f_t,N> lut;

	erf_lut_t() noexcept {
		for (std::size_t i=0;i<N;++i) {
			const auto x = f_t(i)/(N-1) * maxX;
			lut[i] = std::erf(x);
		}
	}

	inline const auto operator()(f_t x) const noexcept {
		const auto sgn = m::sign(x);
		x = m::abs(x) * scale;

		const auto f = m::fract(x);
		const auto idx0 = std::size_t(x);
		const auto idx1 = idx0+1;

		return idx1>=N || idx1==0 ? sgn : sgn * m::mix(lut[idx0],lut[idx1],f);
	}
};

}

/**
 * @brief Evaluates the error function Erf(x) for real x using a precomputed lookup table.
 */
inline f_t erf_lut(const f_t x) noexcept {
	static detail::erf_lut_t lut;
	return lut(x);
}

}
