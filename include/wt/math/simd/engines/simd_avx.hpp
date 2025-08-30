/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <immintrin.h>
#include <bit>
#include <array>
#include <cstdint>


// the boilerplates in this file are designed to provide a consistent SIMD low-level API for AVX targets
// we care about 4 variants: 4xfloat, 8xfloat, 4xdouble, 8xdouble
// TODO: many AVX512 ops take a different approach than AVX2, making it difficult to produce an efficient and uniform API across all variants, especially the comparisons and blendv.
// TODO: some AVX512f variants are quite suboptimal



namespace wt::simd {

/**
 * @brief Low-level SIMD type. Only Width of 4 or 8 and float or double types are supported.
 */
template <typename Fp, std::size_t Width>
struct simd_avx_t {};

// 4 x float. SSE2/3/4
template <>
struct simd_avx_t<float,4> {
    __m128 v;
    template <std::size_t idx>
    [[nodiscard]] inline float extract_static() const noexcept {
        static_assert(idx<4);
        auto x = _mm_extract_ps(v, idx);
        return std::bit_cast<float>(x); 
    }
    [[nodiscard]] inline float extract(std::size_t i) const noexcept {
        auto x = _mm_permutevar_ps(v, _mm_set1_epi32(i));
        return _mm_cvtss_f32(x);
    }
};
// 8 x float. AVX2
template <>
struct simd_avx_t<float,8> {
    __m256 v;
    template <std::size_t idx>
    [[nodiscard]] inline float extract_static() const noexcept {
        static_assert(idx<8);
        auto x = _mm256_extract_epi32(_mm256_castps_si256(v), idx);
        return std::bit_cast<float>(x); 
    }
    [[nodiscard]] inline float extract(std::size_t i) const noexcept {
        auto x = _mm256_permutevar8x32_ps(v, _mm256_set1_epi32(i));
        return _mm256_cvtss_f32(x);
    }
};
// 4 x double. AVX2
template <>
struct simd_avx_t<double,4> {
    __m256d v;
    template <std::size_t idx>
    [[nodiscard]] inline double extract_static() const noexcept {
        static_assert(idx<4);
        auto x = _mm256_extract_epi64(_mm256_castpd_si256(v), idx);
        return std::bit_cast<double>(x); 
    }
    [[nodiscard]] inline double extract(std::size_t i) const noexcept {
        auto x = _mm256_permutexvar_pd(_mm256_set1_epi64x(i), v);
        return _mm256_cvtsd_f64(x);
    }
};
// 8 x double. AVX512
template <>
struct simd_avx_t<double,8> {
    __m512d v;
    template <std::size_t idx>
    [[nodiscard]] inline double extract_static() const noexcept {
        static_assert(idx<8);
        // Extract 128-bit pair, then pick one of the two doubles.
        constexpr int q = idx >> 1;
        constexpr int m = idx & 1;
        auto pair = _mm512_extractf64x2_pd(v, q);
        // select per-lane
        return _mm_cvtsd_f64(_mm_permute_pd(pair,m));
    }
    [[nodiscard]] inline double extract(std::size_t i) const noexcept {
        auto x = _mm512_permutexvar_pd(_mm512_set1_epi64(i), v);
        return _mm512_cvtsd_f64(x);
    }
};

static_assert(alignof(simd_avx_t<float,4>) ==16);
static_assert(alignof(simd_avx_t<float,8>) ==32);
static_assert(alignof(simd_avx_t<double,4>)==32);
// static_assert(alignof(simd_avx_t<double,8>)==64);


static inline simd_avx_t<float,8> cast_to_256(simd_avx_t<float,4> src) noexcept {
    return { _mm256_castps128_ps256(src.v) };
}
static inline simd_avx_t<double,8> cast_to_512d(simd_avx_t<double,4> src) noexcept {
    return { _mm512_castpd256_pd512(src.v) };
}

static inline simd_avx_t<float,4> extract_lower_half(simd_avx_t<float,8> src) noexcept {
    return { _mm256_castps256_ps128(src.v) };
}
static inline simd_avx_t<float,4> extract_upper_half(simd_avx_t<float,8> src) noexcept {
    return { _mm256_extractf128_ps(src.v, 1) };
}
static inline simd_avx_t<float,8> merge_lower_upper(simd_avx_t<float,4> lower, simd_avx_t<float,4> upper) noexcept {
    return { _mm256_insertf128_ps(cast_to_256(lower).v, upper.v, 1) };
}

static inline simd_avx_t<double,4> extract_lower_half(simd_avx_t<double,8> src) noexcept {
    return { _mm512_castpd512_pd256(src.v) };
}
static inline simd_avx_t<double,4> extract_upper_half(simd_avx_t<double,8> src) noexcept {
    return { _mm512_extractf64x4_pd(src.v, 1) };
}
static inline simd_avx_t<double,8> merge_lower_upper(simd_avx_t<double,4> a, simd_avx_t<double,4> b) noexcept {
    return { _mm512_insertf64x4(cast_to_512d(a).v, b.v, 1) };
}

static inline simd_avx_t<double,8> pack_2x256d_to_512d(simd_avx_t<double,4> a, simd_avx_t<double,4> b) noexcept {
    return merge_lower_upper(a,b);
}
static inline void unpack_512d_to_2x256d(simd_avx_t<double,8> src, simd_avx_t<double,4>& a, simd_avx_t<double,4>& b) noexcept {
    a = extract_lower_half(src);
    b = extract_upper_half(src);
}

static inline void loadu(simd_avx_t<float,4>& v, const float* s) noexcept {
    v = { _mm_loadu_ps(s) };
}
static inline void loadu(simd_avx_t<float,8>& v, const float* s) noexcept {
    v = { _mm256_loadu_ps(s) };
}
static inline void loadu(simd_avx_t<double,4>& v, const double* s) noexcept {
    v = { _mm256_loadu_pd(s) };
}
static inline void loadu(simd_avx_t<double,8>& v, const double* s) noexcept {
    v = { _mm512_loadu_pd(s) };
}

static inline void load(simd_avx_t<float,4>& v, const float* s) noexcept {
    v = { _mm_load_ps(s) };
}
static inline void load(simd_avx_t<float,8>& v, const float* s) noexcept {
    v = { _mm256_load_ps(s) };
}
static inline void load(simd_avx_t<double,4>& v, const double* s) noexcept {
    v = { _mm256_load_pd(s) };
}
static inline void load(simd_avx_t<double,8>& v, const double* s) noexcept {
    v = { _mm512_load_pd(s) };
}

static inline void set1(simd_avx_t<float,4>& v, float s) noexcept {
    v = { _mm_set1_ps(s) };
}
static inline void set1(simd_avx_t<float,8>& v, float s) noexcept {
    v = { _mm256_set1_ps(s) };
}
static inline void set1(simd_avx_t<double,4>& v, double s) noexcept {
    v = { _mm256_set1_pd(s) };
}
static inline void set1(simd_avx_t<double,8>& v, double s) noexcept {
    v = { _mm512_set1_pd(s) };
}

static inline void set(simd_avx_t<float,4>& v, const std::array<float,4>& fs) noexcept {
    v = { _mm_set_ps(fs[3],fs[2],fs[1],fs[0]) };
}
static inline void set(simd_avx_t<float,8>& v, const std::array<float,8>& fs) noexcept {
    v = { _mm256_set_ps(fs[7],fs[6],fs[5],fs[4],fs[3],fs[2],fs[1],fs[0]) };
}
static inline void set(simd_avx_t<double,4>& v, const std::array<double,4>& ds) noexcept {
    v = { _mm256_set_pd(ds[3],ds[2],ds[1],ds[0]) };
}
static inline void set(simd_avx_t<double,8>& v, const std::array<double,8>& ds) noexcept {
    v = { _mm512_set_pd(ds[7],ds[6],ds[5],ds[4],ds[3],ds[2],ds[1],ds[0]) };
}

static inline simd_avx_t<float,4> add(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_add_ps(v1.v, v2.v) };
}
static inline simd_avx_t<float,8> add(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_add_ps(v1.v, v2.v) };
}
static inline simd_avx_t<double,4> add(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_add_pd(v1.v, v2.v) };
}
static inline simd_avx_t<double,8> add(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    return { _mm512_add_pd(v1.v, v2.v) };
}

static inline simd_avx_t<float,4> sub(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_sub_ps(v1.v, v2.v) };
}
static inline simd_avx_t<float,8> sub(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_sub_ps(v1.v, v2.v) };
}
static inline simd_avx_t<double,4> sub(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_sub_pd(v1.v, v2.v) };
}
static inline simd_avx_t<double,8> sub(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    return { _mm512_sub_pd(v1.v, v2.v) };
}

static inline simd_avx_t<float,4> mul(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_mul_ps(v1.v, v2.v) };
}
static inline simd_avx_t<float,8> mul(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_mul_ps(v1.v, v2.v) };
}
static inline simd_avx_t<double,4> mul(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_mul_pd(v1.v, v2.v) };
}
static inline simd_avx_t<double,8> mul(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    return { _mm512_mul_pd(v1.v, v2.v) };
}

static inline simd_avx_t<float,4> div(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_div_ps(v1.v, v2.v) };
}
static inline simd_avx_t<float,8> div(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_div_ps(v1.v, v2.v) };
}
static inline simd_avx_t<double,4> div(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_div_pd(v1.v, v2.v) };
}
static inline simd_avx_t<double,8> div(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    return { _mm512_div_pd(v1.v, v2.v) };
}

static inline simd_avx_t<float,4> min(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_min_ps(v1.v, v2.v) };
}
static inline simd_avx_t<float,8> min(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_min_ps(v1.v, v2.v) };
}
static inline simd_avx_t<double,4> min(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_min_pd(v1.v, v2.v) };
}
static inline simd_avx_t<double,8> min(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    return { _mm512_min_pd(v1.v, v2.v) };
}

static inline simd_avx_t<float,4> max(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_max_ps(v1.v, v2.v) };
}
static inline simd_avx_t<float,8> max(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_max_ps(v1.v, v2.v) };
}
static inline simd_avx_t<double,4> max(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_max_pd(v1.v, v2.v) };
}
static inline simd_avx_t<double,8> max(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    return { _mm512_max_pd(v1.v, v2.v) };
}

static inline simd_avx_t<float,4> land(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_and_ps(v1.v, v2.v) };
}
static inline simd_avx_t<float,8> land(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_and_ps(v1.v, v2.v) };
}
static inline simd_avx_t<double,4> land(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_and_pd(v1.v, v2.v) };
}
static inline simd_avx_t<double,8> land(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    return { _mm512_and_pd(v1.v, v2.v) };
}

static inline simd_avx_t<float,4> lor(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_or_ps(v1.v, v2.v) };
}
static inline simd_avx_t<float,8> lor(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_or_ps(v1.v, v2.v) };
}
static inline simd_avx_t<double,4> lor(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_or_pd(v1.v, v2.v) };
}
static inline simd_avx_t<double,8> lor(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    return { _mm512_or_pd(v1.v, v2.v) };
}

static inline simd_avx_t<float,4> lxor(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_xor_ps(v1.v, v2.v) };
}
static inline simd_avx_t<float,8> lxor(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_xor_ps(v1.v, v2.v) };
}
static inline simd_avx_t<double,4> lxor(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_xor_pd(v1.v, v2.v) };
}
static inline simd_avx_t<double,8> lxor(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    return { _mm512_xor_pd(v1.v, v2.v) };
}

static inline simd_avx_t<float,4> fmadd(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2, simd_avx_t<float,4> v3) noexcept {
    return { _mm_fmadd_ps(v1.v, v2.v, v3.v) };
}
static inline simd_avx_t<float,8> fmadd(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2, simd_avx_t<float,8> v3) noexcept {
    return { _mm256_fmadd_ps(v1.v, v2.v, v3.v) };
}
static inline simd_avx_t<double,4> fmadd(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2, simd_avx_t<double,4> v3) noexcept {
    return { _mm256_fmadd_pd(v1.v, v2.v, v3.v) };
}
static inline simd_avx_t<double,8> fmadd(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2, simd_avx_t<double,8> v3) noexcept {
    return { _mm512_fmadd_pd(v1.v, v2.v, v3.v) };
}

static inline simd_avx_t<float,4> fmsub(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2, simd_avx_t<float,4> v3) noexcept {
    return { _mm_fmsub_ps(v1.v, v2.v, v3.v) };
}
static inline simd_avx_t<float,8> fmsub(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2, simd_avx_t<float,8> v3) noexcept {
    return { _mm256_fmsub_ps(v1.v, v2.v, v3.v) };
}
static inline simd_avx_t<double,4> fmsub(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2, simd_avx_t<double,4> v3) noexcept {
    return { _mm256_fmsub_pd(v1.v, v2.v, v3.v) };
}
static inline simd_avx_t<double,8> fmsub(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2, simd_avx_t<double,8> v3) noexcept {
    return { _mm512_fmsub_pd(v1.v, v2.v, v3.v) };
}

static inline simd_avx_t<float,4> sqrt(simd_avx_t<float,4> v) noexcept {
    return { _mm_sqrt_ps(v.v) };
}
static inline simd_avx_t<float,8> sqrt(simd_avx_t<float,8> v) noexcept {
    return { _mm256_sqrt_ps(v.v) };
}
static inline simd_avx_t<double,4> sqrt(simd_avx_t<double,4> v) noexcept {
    return { _mm256_sqrt_pd(v.v) };
}
static inline simd_avx_t<double,8> sqrt(simd_avx_t<double,8> v) noexcept {
    return { _mm512_sqrt_pd(v.v) };
}

static inline simd_avx_t<float,4> abs(simd_avx_t<float,4> v) noexcept {
    return { _mm_andnot_ps(_mm_set1_ps(-.0f), v.v) };
}
static inline simd_avx_t<float,8> abs(simd_avx_t<float,8> v) noexcept {
    return { _mm256_andnot_ps(_mm256_set1_ps(-.0f), v.v) };
}
static inline simd_avx_t<double,4> abs(simd_avx_t<double,4> v) noexcept {
    return { _mm256_andnot_pd(_mm256_set1_pd(-.0), v.v) };
}
static inline simd_avx_t<double,8> abs(simd_avx_t<double,8> v) noexcept {
    return { _mm512_abs_pd(v.v) };
}

static inline simd_avx_t<float,4> floor(simd_avx_t<float,4> v) noexcept {
    return { _mm_floor_ps(v.v) };
}
static inline simd_avx_t<float,8> floor(simd_avx_t<float,8> v) noexcept {
    return { _mm256_floor_ps(v.v) };
}
static inline simd_avx_t<double,4> floor(simd_avx_t<double,4> v) noexcept {
    return { _mm256_floor_pd(v.v) };
}
static inline simd_avx_t<double,8> floor(simd_avx_t<double,8> v) noexcept {
    return { _mm512_floor_pd(v.v) };
}

static inline simd_avx_t<float,4> ceil(simd_avx_t<float,4> v) noexcept {
    return { _mm_ceil_ps(v.v) };
}
static inline simd_avx_t<float,8> ceil(simd_avx_t<float,8> v) noexcept {
    return { _mm256_ceil_ps(v.v) };
}
static inline simd_avx_t<double,4> ceil(simd_avx_t<double,4> v) noexcept {
    return { _mm256_ceil_pd(v.v) };
}
static inline simd_avx_t<double,8> ceil(simd_avx_t<double,8> v) noexcept {
    return { _mm512_ceil_pd(v.v) };
}

template <int mask>
static inline simd_avx_t<float,4> blend(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_blend_ps(v1.v, v2.v, mask) };
}
template <int mask>
static inline simd_avx_t<float,8> blend(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_blend_ps(v1.v, v2.v, mask) };
}
template <int mask>
static inline simd_avx_t<double,4> blend(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_blend_pd(v1.v, v2.v, mask) };
}
template <int mask>
static inline simd_avx_t<double,8> blend(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    return { _mm512_mask_blend_pd(mask, v1.v, v2.v) };
}

static inline simd_avx_t<float,4> blendv(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2, simd_avx_t<float,4> mask) noexcept {
    return { _mm_blendv_ps(v1.v, v2.v, mask.v) };
}
static inline simd_avx_t<float,8> blendv(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2, simd_avx_t<float,8> mask) noexcept {
    return { _mm256_blendv_ps(v1.v, v2.v, mask.v) };
}
static inline simd_avx_t<double,4> blendv(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2, simd_avx_t<double,4> mask) noexcept {
    return { _mm256_blendv_pd(v1.v, v2.v, mask.v) };
}
static inline simd_avx_t<double,8> blendv(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2, simd_avx_t<double,8> mask) noexcept {
    simd_avx_t<double,4> v1l, v1u, v2l, v2u, maskl, masku;
    unpack_512d_to_2x256d(v1, v1l, v1u);
    unpack_512d_to_2x256d(v2, v2l, v2u);
    unpack_512d_to_2x256d(mask, maskl, masku);

    const auto l = blendv(v1l,v2l,maskl);
    const auto u = blendv(v1u,v2u,masku);

    return pack_2x256d_to_512d(l, u);
}

template <int mask>
static inline simd_avx_t<float,8> permute2f(simd_avx_t<float,8> a, simd_avx_t<float,8> b) noexcept {
    return { _mm256_permute2f128_ps(a.v, b.v, mask) };
}
/**
 * @brief Implements `_mm256_permute2f128_ps` but for 8xdouble `simd_avx_t<double,8>`.
 */
template <int mask>
static inline simd_avx_t<double,8> permute2f(simd_avx_t<double,8> a, simd_avx_t<double,8> b) noexcept {
    // TODO: untested

    constexpr auto kz = 
        (unsigned char)(((mask>>3)&1) ? 0 : 0x0F) +
        (unsigned char)(((mask>>7)&1) ? 0 : 0xF0);

    constexpr auto idx_generator = []() {
        std::array<std::int64_t,8> s;
        // lower
        const std::int64_t lselector = (mask&0x2)==0 ? 0 : 1;
        const std::int64_t loffset   = (mask&0x1)==0 ? 0 : 4;
        for (int i=0ul;i<4;++i) s[i] = (lselector<<3) + (loffset+i);
        // upper
        const std::int64_t uselector = ((mask>>4)&0x2)==0 ? 0 : 1;
        const std::int64_t uoffset   = ((mask>>4)&0x1)==0 ? 0 : 4;
        for (int i=0ul;i<4;++i) s[i+4] = (uselector<<3) + (uoffset+i);
        
        return s;
    };

    constexpr auto s = idx_generator();
    const auto idx = _mm512_set_epi64(s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7]);

    return { _mm512_maskz_permutex2var_pd(kz, a.v, idx, b.v) };
}

static inline simd_avx_t<float,4> eq(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_cmp_ps(v1.v, v2.v, _CMP_EQ_OQ) };
}
static inline simd_avx_t<float,8> eq(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_cmp_ps(v1.v, v2.v, _CMP_EQ_OQ) };
}
static inline simd_avx_t<double,4> eq(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_cmp_pd(v1.v, v2.v, _CMP_EQ_OQ) };
}
static inline auto eq(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    simd_avx_t<double,4> v1l, v1u, v2l, v2u;
    unpack_512d_to_2x256d(v1, v1l, v1u);
    unpack_512d_to_2x256d(v2, v2l, v2u);
    return pack_2x256d_to_512d(eq(v1l,v2l), eq(v1u,v2u));
}

static inline simd_avx_t<float,4> neq(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_cmp_ps(v1.v, v2.v, _CMP_NEQ_OQ) };
}
static inline simd_avx_t<float,8> neq(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_cmp_ps(v1.v, v2.v, _CMP_NEQ_OQ) };
}
static inline simd_avx_t<double,4> neq(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_cmp_pd(v1.v, v2.v, _CMP_NEQ_OQ) };
}
static inline auto neq(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    simd_avx_t<double,4> v1l, v1u, v2l, v2u;
    unpack_512d_to_2x256d(v1, v1l, v1u);
    unpack_512d_to_2x256d(v2, v2l, v2u);
    return pack_2x256d_to_512d(neq(v1l,v2l), neq(v1u,v2u));
}

static inline simd_avx_t<float,4> leq(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    const auto t1 = _mm_srli_epi32(_mm_castps_si128(v1.v),31);
    const auto t2 = _mm_srli_epi32(_mm_castps_si128(v2.v),31);
    return { _mm_castsi128_ps(_mm_cmpeq_epi32(t1,t2)) };
}
static inline simd_avx_t<float,8> leq(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    const auto t1 = _mm256_srli_epi32(_mm256_castps_si256(v1.v),31);
    const auto t2 = _mm256_srli_epi32(_mm256_castps_si256(v2.v),31);
    return { _mm256_castsi256_ps(_mm256_cmpeq_epi32(t1,t2)) };
}
static inline simd_avx_t<double,4> leq(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    const auto t1 = _mm256_srli_epi64(_mm256_castpd_si256(v1.v),63);
    const auto t2 = _mm256_srli_epi64(_mm256_castpd_si256(v2.v),63);
    return { _mm256_castsi256_pd(_mm256_cmpeq_epi64(t1,t2)) };
}
static inline auto leq(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    simd_avx_t<double,4> v1l, v1u, v2l, v2u;
    unpack_512d_to_2x256d(v1, v1l, v1u);
    unpack_512d_to_2x256d(v2, v2l, v2u);
    return pack_2x256d_to_512d(leq(v1l,v2l), leq(v1u,v2u));
}

static inline simd_avx_t<float,4> lneq(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    const auto t1 = _mm_srli_epi32(_mm_castps_si128(v1.v),31);
    const auto t2 = _mm_srli_epi32(_mm_castps_si128(v2.v),31);
    return { _mm_castsi128_ps(_mm_cmpgt_epi32(t1,t2)) };
}
static inline simd_avx_t<float,8> lneq(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    const auto t1 = _mm256_srli_epi32(_mm256_castps_si256(v1.v),31);
    const auto t2 = _mm256_srli_epi32(_mm256_castps_si256(v2.v),31);
    return { _mm256_castsi256_ps(_mm256_cmpgt_epi32(t1,t2)) };
}
static inline simd_avx_t<double,4> lneq(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    const auto t1 = _mm256_srli_epi64(_mm256_castpd_si256(v1.v),63);
    const auto t2 = _mm256_srli_epi64(_mm256_castpd_si256(v2.v),63);
    return { _mm256_castsi256_pd(_mm256_cmpgt_epi64(t1,t2)) };
}
static inline auto lneq(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    simd_avx_t<double,4> v1l, v1u, v2l, v2u;
    unpack_512d_to_2x256d(v1, v1l, v1u);
    unpack_512d_to_2x256d(v2, v2l, v2u);
    return pack_2x256d_to_512d(lneq(v1l,v2l), lneq(v1u,v2u));
}

static inline simd_avx_t<float,4> lt(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_cmp_ps(v1.v, v2.v, _CMP_LT_OQ) };
}
static inline simd_avx_t<float,8> lt(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_cmp_ps(v1.v, v2.v, _CMP_LT_OQ) };
}
static inline simd_avx_t<double,4> lt(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_cmp_pd(v1.v, v2.v, _CMP_LT_OQ) };
}
static inline auto lt(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    simd_avx_t<double,4> v1l, v1u, v2l, v2u;
    unpack_512d_to_2x256d(v1, v1l, v1u);
    unpack_512d_to_2x256d(v2, v2l, v2u);
    return pack_2x256d_to_512d(lt(v1l,v2l), lt(v1u,v2u));
}

static inline simd_avx_t<float,4> gt(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_cmp_ps(v1.v, v2.v, _CMP_GT_OQ) };
}
static inline simd_avx_t<float,8> gt(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_cmp_ps(v1.v, v2.v, _CMP_GT_OQ) };
}
static inline simd_avx_t<double,4> gt(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_cmp_pd(v1.v, v2.v, _CMP_GT_OQ) };
}
static inline auto gt(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    simd_avx_t<double,4> v1l, v1u, v2l, v2u;
    unpack_512d_to_2x256d(v1, v1l, v1u);
    unpack_512d_to_2x256d(v2, v2l, v2u);
    return pack_2x256d_to_512d(gt(v1l,v2l), gt(v1u,v2u));
}

static inline simd_avx_t<float,4> le(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_cmp_ps(v1.v, v2.v, _CMP_LE_OQ) };
}
static inline simd_avx_t<float,8> le(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_cmp_ps(v1.v, v2.v, _CMP_LE_OQ) };
}
static inline simd_avx_t<double,4> le(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_cmp_pd(v1.v, v2.v, _CMP_LE_OQ) };
}
static inline auto le(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    simd_avx_t<double,4> v1l, v1u, v2l, v2u;
    unpack_512d_to_2x256d(v1, v1l, v1u);
    unpack_512d_to_2x256d(v2, v2l, v2u);
    return pack_2x256d_to_512d(le(v1l,v2l), le(v1u,v2u));
}

static inline simd_avx_t<float,4> ge(simd_avx_t<float,4> v1, simd_avx_t<float,4> v2) noexcept {
    return { _mm_cmp_ps(v1.v, v2.v, _CMP_GE_OQ) };
}
static inline simd_avx_t<float,8> ge(simd_avx_t<float,8> v1, simd_avx_t<float,8> v2) noexcept {
    return { _mm256_cmp_ps(v1.v, v2.v, _CMP_GE_OQ) };
}
static inline simd_avx_t<double,4> ge(simd_avx_t<double,4> v1, simd_avx_t<double,4> v2) noexcept {
    return { _mm256_cmp_pd(v1.v, v2.v, _CMP_GE_OQ) };
}
static inline auto ge(simd_avx_t<double,8> v1, simd_avx_t<double,8> v2) noexcept {
    simd_avx_t<double,4> v1l, v1u, v2l, v2u;
    unpack_512d_to_2x256d(v1, v1l, v1u);
    unpack_512d_to_2x256d(v2, v2l, v2u);
    return pack_2x256d_to_512d(ge(v1l,v2l), ge(v1u,v2u));
}

static inline bool any4(simd_avx_t<float,4> v) noexcept {
    const auto w = _mm_permute_ps(v.v, 0xe);
    const auto b = _mm_or_ps(v.v,w);

    const auto b0 = !(std::bit_cast<float>(_mm_extract_ps(b,0))==0);
    const auto b1 = !(std::bit_cast<float>(_mm_extract_ps(b,1))==0);

    return b0 || b1;
}
static inline bool any4(simd_avx_t<double,4> v) noexcept {
    const auto w = _mm256_permute_pd(v.v, 0xe);
    const auto b = _mm256_or_pd(v.v,w);

    const auto b0 = !(std::bit_cast<double>(_mm256_extract_epi64(_mm256_castpd_si256(b), 0)));
    const auto b1 = !(std::bit_cast<double>(_mm256_extract_epi64(_mm256_castpd_si256(b), 1)));

    return b0 || b1;
}

static inline bool all4(simd_avx_t<float,4> v) noexcept {
    const auto w = _mm_permute_ps(v.v, 0xe);
    const auto b = _mm_and_ps(v.v,w);

    const auto b0 = !(std::bit_cast<float>(_mm_extract_ps(b,0))==0);
    const auto b1 = !(std::bit_cast<float>(_mm_extract_ps(b,1))==0);

    return b0 && b1;
}
static inline bool all4(simd_avx_t<double,4> v) noexcept {
    const auto w = _mm256_permute_pd(v.v, 0xe);
    const auto b = _mm256_and_pd(v.v,w);

    const auto b0 = !(std::bit_cast<double>(_mm256_extract_epi64(_mm256_castpd_si256(b), 0)));
    const auto b1 = !(std::bit_cast<double>(_mm256_extract_epi64(_mm256_castpd_si256(b), 1)));

    return b0 && b1;
}

}
