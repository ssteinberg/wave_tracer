/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cstdint>
#include <cassert>

#include <wt/util/array.hpp>
#include <wt/util/concepts.hpp>

namespace wt::sampler::sobolld {

static constexpr wt::array_t<std::uint64_t,21> pow3tab = { 
    1,3,9,27,81,243,729,2187,6561,19683,59049,
    177147,531441,1594323,4782969,14348907,43046721,129140163,387420489,1162261467,3486784401
};

/**
 * @brief From "Quad-Optimized Low-Discrepancy Sequences", Victor Ostromoukhov, Nicolas Bonneel, David Coeurjolly, Jean-Claude Iehl, 2024
 *        https://github.com/liris-origami/Quad-Optimized-LDS
 */
template <typename digit_t=std::int8_t, std::size_t N=10>
struct integer3_t {
    wt::array_t<digit_t, N> digits;

    integer3_t(std::uint64_t x) noexcept {
        for (auto i=0ul; i<digits.size(); ++i)
            digits[i] = (x / pow3tab[i]) % 3;
    }
    integer3_t() noexcept : integer3_t(0) {}

    integer3_t(const integer3_t&) noexcept = default;

    auto value(const std::size_t m=N) const noexcept {
        std::uint64_t x = 0;
        for (auto i=0ul; i<m; ++i)
            x+= pow3tab[i] * digits[i];
        return x;
    }

    template <FloatingPoint T>
    auto value_fp(const std::size_t m=N) const noexcept {
        return T(value(m)) / T(pow3tab[m]);
    }

    // x % 3
    static digit_t mod(const int x) {
        static constexpr std::array<digit_t,6> tab_mod3 = { 0, 1, 2, 0, 1, 2 };
        assert(x>=0 && x<6);
        return tab_mod3[x];
    }

    // ( a + (b*c)%3 )%3
    static digit_t fma(const int a, const int b, const int c) {
        static constexpr std::array<digit_t,64> tab_fma4 = {
            0, 0, 0, 0, 0, 1, 2, 0, 0, 2, 
            1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 
            1, 2, 0, 0, 1, 0, 2, 0, 0, 0, 
            0, 0, 2, 2, 2, 0, 2, 0, 1, 0, 
            2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0
        };
        assert(a>=0 && a<3);
        assert(b>=0 && b<3);
        assert(c>=0 && c<3);
        return tab_fma4[a*4*4 + b*4 + c];
    }
};

}
