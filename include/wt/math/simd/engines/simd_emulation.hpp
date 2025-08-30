/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <array>
#include <algorithm>
#include <cassert>

#include <wt/math/common.hpp>

#include <wt/math/simd/common.hpp>


// This file implements simple emulation of typical SIMD operations.


namespace wt::simd {

template <typename Fp, std::size_t Width>
struct simd_emulated_t {
    std::array<Fp,Width> v;

    template <std::size_t idx>
    [[nodiscard]] inline float extract_static() const noexcept { return v[idx]; }
    [[nodiscard]] inline float extract(std::size_t i) const noexcept { return v[i]; }
};

static inline simd_emulated_t<float,8> cast_to_256(simd_emulated_t<float,4> src) noexcept {
    simd_emulated_t<float,8> ret;
    std::copy_n(&src.v[0], 4, &ret.v[0]);
    return ret;
}
static inline simd_emulated_t<double,8> cast_to_512d(simd_emulated_t<double,4> src) noexcept {
    simd_emulated_t<double,8> ret;
    std::copy_n(&src.v[0], 4, &ret.v[0]);
    return ret;
}

static inline simd_emulated_t<float,4> extract_lower_half(simd_emulated_t<float,8> src) noexcept {
    simd_emulated_t<float,4> ret;
    std::copy_n(&src.v[0], 4, &ret.v[0]);
    return ret;
}
static inline simd_emulated_t<float,4> extract_upper_half(simd_emulated_t<float,8> src) noexcept {
    simd_emulated_t<float,4> ret;
    std::copy_n(&src.v[4], 4, &ret.v[0]);
    return ret;
}
static inline simd_emulated_t<float,8> merge_lower_upper(
        simd_emulated_t<float,4> a, simd_emulated_t<float,4> b) noexcept {
    simd_emulated_t<float,8> ret;
    std::copy_n(&a.v[0], 4, &ret.v[0]);
    std::copy_n(&b.v[0], 4, &ret.v[4]);
    return ret;
}

static inline simd_emulated_t<double,4> extract_lower_half(simd_emulated_t<double,8> src) noexcept {
    simd_emulated_t<double,4> ret;
    std::copy_n(&src.v[0], 4, &ret.v[0]);
    return ret;
}
static inline simd_emulated_t<double,4> extract_upper_half(simd_emulated_t<double,8> src) noexcept {
    simd_emulated_t<double,4> ret;
    std::copy_n(&src.v[4], 4, &ret.v[0]);
    return ret;
}
static inline simd_emulated_t<double,8> merge_lower_upper(
        simd_emulated_t<double,4> a, simd_emulated_t<double,4> b) noexcept {
    simd_emulated_t<double,8> ret;
    std::copy_n(&a.v[0], 4, &ret.v[0]);
    std::copy_n(&b.v[0], 4, &ret.v[4]);
    return ret;
}

static inline simd_emulated_t<double,8> pack_2x256d_to_512d(
        simd_emulated_t<double,4> a, simd_emulated_t<double,4> b) noexcept {
    return merge_lower_upper(a,b);
}
static inline void unpack_512d_to_2x256d(simd_emulated_t<double,8> src, simd_emulated_t<double,4>& a, simd_emulated_t<double,4>& b) noexcept {
    std::copy_n(&src.v[0], 4, &a.v[0]);
    std::copy_n(&src.v[4], 4, &b.v[0]);
}

static inline void loadu(simd_emulated_t<float,4>& v, const float* s) noexcept {
    std::copy_n(s, 4, &v.v[0]);
}
static inline void loadu(simd_emulated_t<float,8>& v, const float* s) noexcept {
    std::copy_n(s, 8, &v.v[0]);
}
static inline void loadu(simd_emulated_t<double,4>& v, const double* s) noexcept {
    std::copy_n(s, 4, &v.v[0]);
}
static inline void loadu(simd_emulated_t<double,8>& v, const double* s) noexcept {
    std::copy_n(s, 8, &v.v[0]);
}

static inline void load(simd_emulated_t<float,4>& v, const float* s) noexcept {
    std::copy_n(s, 4, &v.v[0]);
}
static inline void load(simd_emulated_t<float,8>& v, const float* s) noexcept {
    std::copy_n(s, 8, &v.v[0]);
}
static inline void load(simd_emulated_t<double,4>& v, const double* s) noexcept {
    std::copy_n(s, 4, &v.v[0]);
}
static inline void load(simd_emulated_t<double,8>& v, const double* s) noexcept {
    std::copy_n(s, 8, &v.v[0]);
}

static inline void set1(simd_emulated_t<float,4>& v, float s) noexcept {
    v.v.fill(s);
}
static inline void set1(simd_emulated_t<float,8>& v, float s) noexcept {
    v.v.fill(s);
}
static inline void set1(simd_emulated_t<double,4>& v, double s) noexcept {
    v.v.fill(s);
}
static inline void set1(simd_emulated_t<double,8>& v, double s) noexcept {
    v.v.fill(s);
}

static inline void set(simd_emulated_t<float,4>& v, const std::array<float,4>& fs) noexcept {
    v.v = fs;
}
static inline void set(simd_emulated_t<float,8>& v, const std::array<float,8>& fs) noexcept {
    v.v = fs;
}
static inline void set(simd_emulated_t<double,4>& v, const std::array<double,4>& ds) noexcept {
    v.v = ds;
}
static inline void set(simd_emulated_t<double,8>& v, const std::array<double,8>& ds) noexcept {
    v.v = ds;
}

template <typename Fp,std::size_t W>
static inline auto add(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = v1.v[n]+v2.v[n];
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto sub(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = v1.v[n]-v2.v[n];
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto mul(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = v1.v[n]*v2.v[n];
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto div(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = v1.v[n]/v2.v[n];
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto min(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = m::min(v1.v[n],v2.v[n]);
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto max(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = m::max(v1.v[n],v2.v[n]);
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto land(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    using Int = std::conditional_t<std::is_same_v<Fp, float>, std::uint32_t, std::uint64_t>;
    static_assert(sizeof(Int) == sizeof(Fp));

    decltype(v1) ret;
    for (auto n=0ul;n<W;++n)
        ret.v[n] = m::reinterpret_bits<Fp>(
                m::reinterpret_bits<Int>(v1.v[n]) & m::reinterpret_bits<Int>(v2.v[n]));
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto lor(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    using Int = std::conditional_t<std::is_same_v<Fp, float>, std::uint32_t, std::uint64_t>;
    static_assert(sizeof(Int) == sizeof(Fp));

    decltype(v1) ret;
    for (auto n=0ul;n<W;++n)
        ret.v[n] = m::reinterpret_bits<Fp>(
                m::reinterpret_bits<Int>(v1.v[n]) | m::reinterpret_bits<Int>(v2.v[n]));
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto lxor(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    using Int = std::conditional_t<std::is_same_v<Fp, float>, std::uint32_t, std::uint64_t>;
    static_assert(sizeof(Int) == sizeof(Fp));

    decltype(v1) ret;
    for (auto n=0ul;n<W;++n)
        ret.v[n] = m::reinterpret_bits<Fp>(
                m::reinterpret_bits<Int>(v1.v[n]) ^ m::reinterpret_bits<Int>(v2.v[n]));
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto fmadd(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2, simd_emulated_t<Fp,W> v3) noexcept {
    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = m::fma(v1.v[n],v2.v[n],v3.v[n]);
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto fmsub(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2, simd_emulated_t<Fp,W> v3) noexcept {
    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = v1.v[n]*v2.v[n] - v3.v[n];
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto sqrt(simd_emulated_t<Fp,W> v) noexcept {
    for (auto& f : v) f = m::sqrt(f);
    return v;
}

template <typename Fp,std::size_t W>
static inline auto abs(simd_emulated_t<Fp,W> v) noexcept {
    for (auto& f : v) f = m::abs(f);
    return v;
}

template <typename Fp,std::size_t W>
static inline auto floor(simd_emulated_t<Fp,W> v) noexcept {
    for (auto& f : v) f = m::floor(f);
    return v;
}

template <typename Fp,std::size_t W>
static inline auto ceil(simd_emulated_t<Fp,W> v) noexcept {
    for (auto& f : v) f = m::ceil(f);
    return v;
}

template <int mask,typename Fp,std::size_t W>
static inline auto blend(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = ((mask>>n)&1) ? v2.v[n] : v1.v[n];
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto blendv(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2,
                          simd_emulated_t<Fp,W> mask) noexcept {
    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = m::signbit(mask.v[n]) ? v2.v[n] : v1.v[n];
    return ret;
}

template <int mask,typename Fp,std::size_t W>
static inline auto permute2f(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    decltype(v1) ret;
    // lower
    switch (mask&0xF) {
    case 0: std::copy_n(&v1.v[0], 4, &ret.v[0]); break;
    case 1: std::copy_n(&v1.v[4], 4, &ret.v[0]); break;
    case 2: std::copy_n(&v2.v[0], 4, &ret.v[0]); break;
    case 3: std::copy_n(&v2.v[4], 4, &ret.v[0]); break;
    default: std::fill(&ret.v[0],&ret.v[4],0);    break;
    }
    // upper
    switch ((mask>>4)&0xF) {
    case 0: std::copy_n(&v1.v[0], 4, &ret.v[4]); break;
    case 1: std::copy_n(&v1.v[4], 4, &ret.v[4]); break;
    case 2: std::copy_n(&v2.v[0], 4, &ret.v[4]); break;
    case 3: std::copy_n(&v2.v[4], 4, &ret.v[4]); break;
    default: std::fill(&ret.v[4],&ret.v[8],0);    break;
    }
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto eq(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    constexpr auto mask = scalar_logical_true_value<Fp>{}();

    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = v1.v[n]==v2.v[n] ? mask : 0;
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto neq(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    constexpr auto mask = scalar_logical_true_value<Fp>{}();

    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = v1.v[n]!=v2.v[n] ? mask : 0;
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto leq(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    constexpr auto mask = scalar_logical_true_value<Fp>{}();

    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = m::signbit(v1.v[n])==m::signbit(v2.v[n]) ? mask : 0;
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto lneq(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    constexpr auto mask = scalar_logical_true_value<Fp>{}();

    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = m::signbit(v1.v[n])!=m::signbit(v2.v[n]) ? mask : 0;
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto lt(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    constexpr auto mask = scalar_logical_true_value<Fp>{}();

    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = v1.v[n]<v2.v[n] ? mask : 0;
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto gt(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    constexpr auto mask = scalar_logical_true_value<Fp>{}();

    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = v1.v[n]>v2.v[n] ? mask : 0;
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto le(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    constexpr auto mask = scalar_logical_true_value<Fp>{}();

    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = v1.v[n]<=v2.v[n] ? mask : 0;
    return ret;
}

template <typename Fp,std::size_t W>
static inline auto ge(simd_emulated_t<Fp,W> v1, simd_emulated_t<Fp,W> v2) noexcept {
    constexpr auto mask = scalar_logical_true_value<Fp>{}();

    decltype(v1) ret;
    for (auto n=0ul;n<W;++n) ret.v[n] = v1.v[n]>=v2.v[n] ? mask : 0;
    return ret;
}

template <typename Fp>
static inline bool any4(simd_emulated_t<Fp,4> v) noexcept {
    return m::signbit(v.v[0]) || m::signbit(v.v[1]) || m::signbit(v.v[2]) || m::signbit(v.v[3]);
}

template <typename Fp>
static inline bool all4(simd_emulated_t<Fp,4> v) noexcept {
    return m::signbit(v.v[0]) && m::signbit(v.v[1]) && m::signbit(v.v[2]) && m::signbit(v.v[3]);
}

}
