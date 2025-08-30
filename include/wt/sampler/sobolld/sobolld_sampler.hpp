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

#include <vector>

#include <wt/math/common.hpp>
#include <wt/util/array.hpp>

#include "integer3.hpp"
#include "irreducible_gf3.hpp"

namespace wt::sampler::sobolld {

template <std::size_t dims>
struct sobolls_sampler {
    static constexpr auto D = dims;
    static constexpr auto N = irreducible_gf3_t::sobolld_gfn_seq_length;

    static_assert(D<irreducible_gf3_t::sobolld_irreducible_entries);

    using digit_t = irreducible_gf3_t::digit_t;
    using matrix_t = std::vector<wt::array_t<digit_t, N>>;

    using uint_t = std::uint64_t;
    using int3_t = integer3_t<digit_t, N>;

    std::array<matrix_t, D> matrix;

    static constexpr auto sample_count_for_mat_size(std::size_t M) noexcept {
        return (std::size_t)m::pow<std::size_t>(3, M);
    }
    static constexpr auto mat_size_for_sample_count(std::size_t P) noexcept {
        return std::size_t(m::ceil(m::log(f_t(P))/m::log(f_t(3))) + f_t(.5));
    }
    static constexpr auto max_mat_size() noexcept { return N; }

    sobolls_sampler(std::size_t mat_size, const irreducible_gf3_t& gf3) noexcept {
        assert(mat_size<=N);

        for (auto d=0ul; d<D; ++d) {
            auto mk = gf3.sobol_mk[d+1];
            gf3.generate_mkgf3(gf3.sobol_aj[d+1], gf3.sobol_sj[d+1], mk.data(), 3);
            
            matrix[d] = gen_mat(mk, mat_size);
        }
    }

    template <FloatingPoint T, typename Rand>
    inline auto generate_points(Rand rng,
                                std::size_t max_sample_count = limits<std::size_t>::max()) const noexcept {
        const auto M = matrix[0].size();
        const auto sample_count = m::min(sample_count_for_mat_size(M), max_sample_count);
        
        std::vector<T> samples;
        samples.reserve(sample_count*D);
        
        wt::array_t<uint_t,D> seeds;
        for (auto d=0ul; d<D; ++d)
            seeds[d] = rng();
        
        wt::array_t<int3_t,D> x3;
        wt::array_t<int3_t,D> p3;
        
        // first owen point, all dimensions
        for (auto d=0ul; d<D; ++d) {
            int3_t y = scramble_base3(x3[d], seeds[d], M);
            const auto sample = y.template value_fp<T>(M);
            samples.push_back(sample);
        }
        // all other points
        for (auto i=1ul; i<sample_count; ++i) {
            auto i3 = int3_t{ (uint_t)i };
            
            for (auto d=0ul; d<D; ++d) {
                const int3_t x = point3_digits(matrix[d], i3, p3[d], x3[d]);
                const int3_t y = scramble_base3(x, seeds[d], M);
                const auto sample = y.template value_fp<T>(M);
                samples.push_back(sample);
            }
        }

        return samples;
    }

private:
    struct rng_t {
        uint_t n{};
        uint_t key{};
        
        rng_t(const uint_t s) : 
            key((s << 1) | 1u) {}
        
        rng_t& index(const uint_t i) {
            n = i;
            return *this;
        }
        uint_t sample() {
            return hash(++n * key);
        }
        
        uint_t sample_range(const uint_t range) {
            // Efficiently Generating a Number in a Range
            // cf http://www.pcg-random.org/posts/bounded-rands.html
            uint_t divisor= ((-range) / range) + 1; // (2^32) / range
            if(divisor == 0) return 0;
            
            while(true) {
                uint_t x = sample() / divisor;
                if(x<range) return x;
            }
        }
        
        inline uint_t hash(uint_t x) {
            x ^= x >> 16;
            x *= 0x21f0aaad;
            x ^= x >> 15;
            x *= 0xd35a2d97;
            x ^= x >> 15;
            return x;
        }
        // cf "hash prospector" https://github.com/skeeto/hash-prospector/blob/master/README.md
    };

private:
    static inline auto gen_mat(const wt::array_t<digit_t,32>& sobol_mk, const std::size_t mat_size) noexcept {
        matrix_t m;
        m.resize(mat_size);
        for (auto i=0ul; i<mat_size; ++i) {
            const int val = sobol_mk[i];
            const auto len = i+1;
            
            wt::array_t<digit_t, N> digits;
            assert(len<=N);
            irreducible_gf3_t::to_digit_array(digits.data(), val,3,len);
            for (auto j=0ul; j<len; ++j)
                m[len-j-1][i] = digits[j];
        }

        return m;
    }

    /**
    * @brief Generate a sobol point by incrementally modifying the previous point.
    */
    static inline int3_t point3_digits(const matrix_t& matrix,
                                       const int3_t& i3, 
                                       int3_t& p3, 
                                       int3_t& x3) noexcept {
        const auto M = matrix.size();
        for(auto k=0ul; k<M; ++k) {
            // find modified digits : previous index (i-1) and index (i)
            if(p3.digits[k] != i3.digits[k]) {
                auto d = digit_t(i3.digits[k]) - digit_t(p3.digits[k]);
                d = int3_t::mod(d+3);
                
                // update previous point
                for (auto j=0ul; j<M; ++j)
                    x3.digits[j] = int3_t::fma(x3.digits[j], d, matrix[M-1 - j][k]);
            }
        }

        // update previous index
        p3 = i3;
        return x3;
    }

    /**
     * @brief Nested uniform scrambling / owen scrambling
     */
    static inline int3_t scramble_base3(const int3_t& a3, const uint_t seed, const uint_t ndigits) noexcept {
        // all random permutations of base-3 digits
        static constexpr std::int8_t scramble[6][3] = {
            {0, 1, 2},
            {0, 2, 1},
            {1, 0, 2},
            {1, 2, 0},
            {2, 0, 1},
            {2, 1, 0},
        };
        
        // counter-based random number generator
        rng_t rng(seed);
        
        int3_t b3;
        uint_t node_index= 0;                                   // start at the root node
        for(auto i=0ul; i<ndigits; ++i) {
            uint_t flip = rng.index(node_index).sample_range(6);   // get a random permutation using the node index
            uint_t digit = a3.digits[ndigits-1 - i];               // get a digit
            b3.digits[ndigits-1 - i] = scramble[flip][digit];        // store the permuted digit
            
            node_index = 3*node_index +1 + digit;                    // continue walking the permutation tree
            // heap layout, root i= 0, childs 3i+1, 3i+2, 3i+3
        }
        
        return b3;
    }
};

}
