/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/defs.hpp>
#include <wt/math/quantity/math.hpp>
#include "wide_vector.hpp"


namespace wt {

template <Quantity Q, std::size_t W, std::size_t N>
inline auto operator+(wide_vector<W,N,Q> v) noexcept {
    return v;
}
template <Quantity Q, std::size_t W, std::size_t N>
inline auto operator-(wide_vector<W,N,Q> v) noexcept {
    const auto s0 = wide_vector<W,1,Q>::zero().simd_native(0);
    for (auto i=0ul;i<N;++i)
        v.simd_native(i) = simd::sub(s0,v.simd_native(i));
    return v;
}

template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator+(const wide_vector<W,N,Q1>& v1, const wide_vector<W,N,Q2>& v2) noexcept {
    using R = decltype(std::declval<Q1>()+std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::add(v1.simd_native(i),v2.simd_native(i));
    return ret;
}
template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator-(const wide_vector<W,N,Q1>& v1, const wide_vector<W,N,Q2>& v2) noexcept {
    using R = decltype(std::declval<Q1>()-std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::sub(v1.simd_native(i),v2.simd_native(i));
    return ret;
}

template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator*(const wide_vector<W,N,Q1>& v1, const wide_vector<W,N,Q2>& v2) noexcept {
    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::mul(v1.simd_native(i),v2.simd_native(i));
    return ret;
}
template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator*(const wide_vector<W,N,Q1>& v1, const wide_vector<W,1,Q2>& v2) noexcept requires (N>1) {
    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::mul(v1.simd_native(i),v2.simd_native(0));
    return ret;
}
template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator*(const wide_vector<W,1,Q1>& v1, const wide_vector<W,N,Q2>& v2) noexcept requires (N>1) {
    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::mul(v1.simd_native(0),v2.simd_native(i));
    return ret;
}

template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator/(const wide_vector<W,N,Q1>& v1, const wide_vector<W,N,Q2>& v2) noexcept {
    using R = decltype(std::declval<Q1>()/std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::div(v1.simd_native(i),v2.simd_native(i));
    return ret;
}

template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator*(const wide_vector<W,N,Q1>& v1, const qvec<N,Q2>& s) noexcept {
    const auto v2 = wide_vector<W,N,Q2>{ s };

    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::mul(v1.simd_native(i),v2.simd_native(i));
    return ret;
}
template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator*(const wide_vector<W,N,Q1>& v1, const qvec<1,Q2>& s) noexcept requires (N>1) {
    const auto v2 = wide_vector<W,N,Q2>{ s };

    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::mul(v1.simd_native(i),v2.simd_native(0));
    return ret;
}
template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator*(const wide_vector<W,1,Q1>& v1, const qvec<N,Q2>& s) noexcept requires (N>1) {
    const auto v2 = wide_vector<W,N,Q2>{ s };

    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::mul(v1.simd_native(0),v2.simd_native(i));
    return ret;
}

template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator*(const qvec<N,Q1>& s, const wide_vector<W,N,Q2>& v2) noexcept {
    const auto v1 = wide_vector<W,N,Q1>{ s };

    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::mul(v1.simd_native(i),v2.simd_native(i));
    return ret;
}
template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator*(const qvec<N,Q1>& s, const wide_vector<W,1,Q2>& v2) noexcept requires (N>1) {
    const auto v1 = wide_vector<W,N,Q1>{ s };

    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::mul(v1.simd_native(i),v2.simd_native(0));
    return ret;
}
template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator*(const qvec<1,Q1>& s, const wide_vector<W,N,Q2>& v2) noexcept requires (N>1) {
    const auto v1 = wide_vector<W,N,Q1>{ s };

    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::mul(v1.simd_native(0),v2.simd_native(i));
    return ret;
}

template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto operator/(const wide_vector<W,N,Q1>& v1, const qvec<N,Q2>& s) noexcept {
    const auto v2 = wide_vector<W,N,Q2>{ s };

    using R = decltype(std::declval<Q1>()/std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::div(v1.simd_native(i),v2.simd_native(i));
    return ret;
}

template <Quantity Q1, Scalar Q2, std::size_t W, std::size_t N>
    requires ( is_quantity_v<Q2> && std::is_same_v<typename wide_vector<W,N,Q1>::Fp, typename Q2::rep>) ||
             (!is_quantity_v<Q2> && std::is_same_v<typename wide_vector<W,N,Q1>::Fp, Q2>)
inline auto operator*(const wide_vector<W,N,Q1>& v1, Q2 s) noexcept {
    using SQ = std::conditional_t<is_quantity_v<Q2>, Q2, simd::unitless>;
    const auto v2 = wide_vector<W,N,SQ>::from_scalar(s);

    using R = decltype(std::declval<Q1>()*s);
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::mul(v1.simd_native(i),v2.simd_native(i));
    return ret;
}
template <Scalar Q1, Quantity Q2, std::size_t W, std::size_t N>
    requires ( is_quantity_v<Q1> && std::is_same_v<typename wide_vector<W,N,Q2>::Fp, typename Q1::rep>) ||
             (!is_quantity_v<Q1> && std::is_same_v<typename wide_vector<W,N,Q2>::Fp, Q1>)
inline auto operator*(Q1 s, const wide_vector<W,N,Q2>& v2) noexcept {
    using SQ = std::conditional_t<is_quantity_v<Q1>, Q1, simd::unitless>;
    const auto v1 = wide_vector<W,N,SQ>::from_scalar(s);

    using R = decltype(s*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::mul(v1.simd_native(i),v2.simd_native(i));
    return ret;
}
template <Quantity Q1, Scalar Q2, std::size_t W, std::size_t N>
    requires ( is_quantity_v<Q2> && std::is_same_v<typename wide_vector<W,N,Q1>::Fp, typename Q2::rep>) ||
             (!is_quantity_v<Q2> && std::is_same_v<typename wide_vector<W,N,Q1>::Fp, Q2>)
inline auto operator/(const wide_vector<W,N,Q1>& v1, Q2 s) noexcept {
    using SQ = std::conditional_t<is_quantity_v<Q2>, Q2, simd::unitless>;
    const auto v2 = wide_vector<W,N,SQ>::from_scalar(s);

    using R = decltype(std::declval<Q1>()/s);
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::div(v1.simd_native(i),v2.simd_native(i));
    return ret;
}

template <Quantity Q1, std::size_t W, std::size_t N, Unit U>
inline auto operator*(const wide_vector<W,N,Q1>& v1, U u) noexcept {
    using R = decltype(std::declval<Q1>()*u);
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = v1.simd_native(i);
    return ret;
}
template <Quantity Q2, std::size_t W, std::size_t N, Unit U>
inline auto operator*(U u, const wide_vector<W,N,Q2>& v2) noexcept {
    using R = decltype(u*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = v2.simd_native(i);
    return ret;
}
template <Quantity Q1, std::size_t W, std::size_t N, Unit U>
inline auto operator/(const wide_vector<W,N,Q1>& v1, U u) noexcept {
    using R = decltype(std::declval<Q1>()/u);
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = v1.simd_native(i);
    return ret;
}

template <std::size_t W, std::size_t N>
inline auto operator&&(const wide_vector<W,N,simd::bool_mask>& a, const wide_vector<W,N,simd::bool_mask>& b) noexcept {
    wide_vector<W,N,simd::bool_mask> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::land(a.simd_native(i), b.simd_native(i));
    return ret;
}
template <std::size_t W, std::size_t N>
inline auto operator||(const wide_vector<W,N,simd::bool_mask>& a, const wide_vector<W,N,simd::bool_mask>& b) noexcept {
    wide_vector<W,N,simd::bool_mask> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::lor(a.simd_native(i), b.simd_native(i));
    return ret;
}

template <std::size_t W, std::size_t N>
inline auto operator!(const wide_vector<W,N,simd::bool_mask>& v) noexcept {
    const auto mask_true = wide_vector<W,1,simd::bool_mask>::mask_true();

    wide_vector<W,N,simd::unitless> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::lxor(v.simd_native(i), mask_true.simd_native(0));
    return ret;
}

}


namespace wt::m {

template <Quantity Q, std::size_t W, std::size_t N>
inline auto sqrt(const wide_vector<W,N,Q>& v) noexcept {
    using R = decltype(sqrt(std::declval<Q>()));
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::sqrt(v.simd_native(i));
    return ret;
}

template <Quantity Q, std::size_t W, std::size_t N>
inline auto abs(const wide_vector<W,N,Q>& v) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::abs(v.simd_native(i));
    return ret;
}

template <Quantity Q, std::size_t W, std::size_t N>
inline auto floor(const wide_vector<W,N,Q>& v) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::floor(v.simd_native(i));
    return ret;
}

template <Quantity Q, std::size_t W, std::size_t N>
inline auto ceil(const wide_vector<W,N,Q>& v) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::ceil(v.simd_native(i));
    return ret;
}

/**
 * @brief Wide `min()`. Note that depending on the used instruction set, results might differ: AVX does NOT follow IEEE754 standard.
 */
template <Quantity Q, std::size_t W, std::size_t N>
inline auto min(const wide_vector<W,N,Q>& v1, const wide_vector<W,N,Q>& v2) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::min(v1.simd_native(i),v2.simd_native(i));
    return ret;
}
/**
 * @brief Wide `max()`. Note that depending on the used instruction set, results might differ: AVX does NOT follow IEEE754 standard.
 */
template <Quantity Q, std::size_t W, std::size_t N>
inline auto max(const wide_vector<W,N,Q>& v1, const wide_vector<W,N,Q>& v2) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::max(v1.simd_native(i),v2.simd_native(i));
    return ret;
}
/**
 * @brief Wide `min()`. Note that depending on the used instruction set, results might differ: AVX does NOT follow IEEE754 standard.
 */
template <Quantity Q, std::size_t W, std::size_t N>
inline auto min(const wide_vector<W,N,Q>& v1,
                const wide_vector<W,N,Q>& v2,
                const wide_vector<W,N,Q>& v3) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::min(simd::min(v1.simd_native(i),v2.simd_native(i)),
                                       v3.simd_native(i));
    return ret;
}
/**
 * @brief Wide `max()`. Note that depending on the used instruction set, results might differ: AVX does NOT follow IEEE754 standard.
 */
template <Quantity Q, std::size_t W, std::size_t N>
inline auto max(const wide_vector<W,N,Q>& v1,
                const wide_vector<W,N,Q>& v2,
                const wide_vector<W,N,Q>& v3) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::max(simd::max(v1.simd_native(i),v2.simd_native(i)),
                                       v3.simd_native(i));
    return ret;
}
/**
 * @brief Wide `min()`. Note that depending on the used instruction set, results might differ: AVX does NOT follow IEEE754 standard.
 */
template <Quantity Q, std::size_t W, std::size_t N>
inline auto min(const wide_vector<W,N,Q>& v1,
                const wide_vector<W,N,Q>& v2,
                const wide_vector<W,N,Q>& v3,
                const wide_vector<W,N,Q>& v4) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::min(simd::min(v1.simd_native(i),v2.simd_native(i)),
                                       simd::min(v3.simd_native(i),v4.simd_native(i)));
    return ret;
}
/**
 * @brief Wide `max()`. Note that depending on the used instruction set, results might differ: AVX does NOT follow IEEE754 standard.
 */
template <Quantity Q, std::size_t W, std::size_t N>
inline auto max(const wide_vector<W,N,Q>& v1,
                const wide_vector<W,N,Q>& v2,
                const wide_vector<W,N,Q>& v3,
                const wide_vector<W,N,Q>& v4) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::max(simd::max(v1.simd_native(i),v2.simd_native(i)),
                                       simd::max(v3.simd_native(i),v4.simd_native(i)));
    return ret;
}

template <Quantity Q, std::size_t W, std::size_t N>
inline auto clamp(const wide_vector<W,N,Q>& v,
                  const wide_vector<W,N,Q>& min, const wide_vector<W,N,Q>& max) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) =
            simd::min(simd::max(v.simd_native(i), min.simd_native(i)),
                      max.simd_native(i));
    return ret;
}

template <Quantity Q, std::size_t W, std::size_t N>
inline auto clamp01(const wide_vector<W,N,Q>& v) noexcept {
    const auto s0 = wide_vector<W,1,Q>::zero().simd_native(0);
    const auto s1 = wide_vector<W,1,Q>::one().simd_native(0);

    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) =
            simd::min(simd::max(v.simd_native(i), s0),
                      s1);
    return ret;
}

template <Quantity Q, std::size_t W, std::size_t N>
inline auto mix(const wide_vector<W,N,Q>& a, const wide_vector<W,N,Q>& b,
                const wide_vector<W,N,simd::unitless>& x) noexcept {
    const auto one = wide_vector<W,N,simd::unitless>::one();
    return a * (one - x) + b * x;
}
template <Quantity Q, std::size_t W, std::size_t N>
inline auto mix(const wide_vector<W,N,Q>& a, const wide_vector<W,N,Q>& b,
                const typename Q::rep& x) noexcept {
    return a * (Q::rep(1) - x) + b * x;
}

/**
 * @brief This implements the AVX `blend` op. Selects from `a` and `b` using the immediate control mask `imm`.
 *        Each bit of the control mask selects the corresponding component from `a` or `b`.
 */
template <int imm, Quantity Q, std::size_t W, std::size_t N>
inline auto select(const wide_vector<W,N,Q>& a, const wide_vector<W,N,Q>& b) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::blend<imm>(a.simd_native(i),b.simd_native(i));
    return ret;
}

/**
 * @brief This implements the AVX `blendv` op. Selects from `a` and `b` using the boolean mask `mask`.
 */
template <Quantity Q, std::size_t W, std::size_t N>
inline auto selectv(const wide_vector<W,N,Q>& a, const wide_vector<W,N,Q>& b,
                    const wide_vector<W,N,simd::bool_mask>& mask) noexcept {
    wide_vector<W,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::blendv(a.simd_native(i),b.simd_native(i),mask.simd_native(i));
    return ret;
}

/**
 * @brief This implements an op similar to AVX's `permute2f128` (for 256bit single-precision input). Shuffles lower and upper halfs of the wide vector using control mask `imm`.
 *        The value of lower/upper 4-bits of `imm` select the source for the lower/upper half of the return value:
 *        * `imm==0` : lower half of a
 *        * `imm==1` : upper half of a
 *        * `imm==2` : lower half of b
 *        * `imm==3` : upper half of b
 *        * `imm==8` : 0
 *        Only supported for 8-wide wide vectors.
 */
template <int imm, Quantity Q, std::size_t N>
inline auto permute2f(const wide_vector<8,N,Q>& a, const wide_vector<8,N,Q>& b) noexcept {
    wide_vector<8,N,Q> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::permute2f<imm>(a.simd_native(i),b.simd_native(i));
    return ret;
}

/**
 * @brief Wide fused-multiply-add: `v1*v2+v3`.
 */
template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto fma(const wide_vector<W,N,Q1>& v1, const wide_vector<W,N,Q2>& v2,
                const wide_vector<W,N,decltype(std::declval<Q1>()*std::declval<Q2>())>& v3) noexcept {
    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::fmadd(v1.simd_native(i), v2.simd_native(i), v3.simd_native(i));
    return ret;
}
/**
 * @brief Wide fused-multiply-substract: `v1*v2-v3`.
 */
template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto fms(const wide_vector<W,N,Q1>& v1, const wide_vector<W,N,Q2>& v2,
                const wide_vector<W,N,decltype(std::declval<Q1>()*std::declval<Q2>())>& v3) noexcept {
    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    wide_vector<W,N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret.simd_native(i) = simd::fmsub(v1.simd_native(i), v2.simd_native(i), v3.simd_native(i));
    return ret;
}


namespace eft {

/**
 * @brief EFT SIMD helper, computes the product `a*b`; stores the computation error in `err`.
 */
template <Quantity Q, std::size_t W, std::size_t N>
inline auto two_prod(wide_vector<W,N,Q>& err,
                     const wide_vector<W,N,Q>& a,
                     const wide_vector<W,N,Q>& b) noexcept {
    const auto prod = a*b;
    err = fms(a, b, prod);
    return prod;
}

/**
 * @brief EFT SIMD helper, computes the sum `a+b`; stores the computation error in `err`.
 */
template <Quantity Q, std::size_t W, std::size_t N>
inline auto two_sum(wide_vector<W,N,Q>& err,
                    const wide_vector<W,N,Q>& a,
                    const wide_vector<W,N,Q>& b) noexcept {
    const auto sum = a+b;
    const auto e1 = sum-a;
    const auto e2 = sum-e1;
    err = (b-e1) + (a-e2);
    return sum;
}

/**
 * @brief Computes the difference a*b-c*d.
 */
template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto diff_prod(const wide_vector<W,N,Q1>& a, 
                      const wide_vector<W,N,Q2>& b, 
                      const wide_vector<W,N,Q1>& c, 
                      const wide_vector<W,N,Q2>& d) noexcept {
    const auto cd = c*d;
    const auto diff = fms(a,b,cd);
    const auto err = fms(c,d,cd);
    return diff-err;
}
/**
 * @brief Computes the sum a*b+c*d.
 */
template <Quantity Q1, Quantity Q2, std::size_t W, std::size_t N>
inline auto sum_prod(const wide_vector<W,N,Q1>& a, 
                     const wide_vector<W,N,Q2>& b, 
                     const wide_vector<W,N,Q1>& c, 
                     const wide_vector<W,N,Q2>& d) noexcept {
    const auto cd = c*d;
    const auto diff = fma(a,b,cd);
    const auto err = fms(c,d,cd);
    return diff+err;
}

template <Quantity Q, std::size_t W>
inline auto dot(const wide_vector<W,3,Q>& u, const wide_vector<W,3,Q>& v) noexcept {
    wide_vector<W,3,Q> dot, err, err1, err2, temp;

    dot  = two_prod(err, u.x(), v.x());
    temp = two_prod(err1, u.y(), v.y());
    dot  = two_sum(err2, dot, temp);
    err  = err + (err1 + err2);
    temp = two_prod(err1, u.z(), v.z());
    dot  = two_sum(err2, dot, temp);
    err  = err + (err1 + err2);

    return dot + err;
}

}



template <Quantity Q1, Quantity Q2, std::size_t W>
inline auto dot(const wide_vector<W,3,Q1>& u, const wide_vector<W,3,Q2>& v) noexcept {
    auto sum = u.x() * v.x();
    sum = fma(u.y(), v.y(), sum);
    return fma(u.z(), v.z(), sum);
}

template <Quantity Q1, Quantity Q2, std::size_t W>
inline auto cross(const wide_vector<W,3,Q1>& u, const wide_vector<W,3,Q2>& v) noexcept {
    using R = decltype(std::declval<Q1>()*std::declval<Q2>());
    return wide_vector<W,3,R>{
        eft::diff_prod(u.y(),v.z(), u.z(),v.y()),
        eft::diff_prod(u.z(),v.x(), u.x(),v.z()),
        eft::diff_prod(u.x(),v.y(), u.y(),v.x())
    };
}

template <std::size_t N>
inline auto any(const wide_vector<4,N,simd::bool_mask>& test) noexcept requires (N==1) {
    return simd::any4(test.simd_native(0));
}
template <std::size_t N>
inline auto any(const wide_vector<4,N,simd::bool_mask>& test) noexcept requires (N>1) {
    bvec<N> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = simd::any4(test.simd_native(i));
    return ret;
}
template <std::size_t N>
inline auto any(const wide_vector<8,N,simd::bool_mask>& test) noexcept {
    return any(test.extract_lower_half() || test.extract_upper_half());
}

template <std::size_t N>
inline auto all(const wide_vector<4,N,simd::bool_mask>& test) noexcept requires (N==1) {
    return simd::all4(test.simd_native(0));
}
template <std::size_t N>
inline auto all(const wide_vector<4,N,simd::bool_mask>& test) noexcept requires (N>1) {
    bvec<N> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = simd::all4(test.simd_native(i));
    return ret;
}
template <std::size_t N>
inline auto all(const wide_vector<8,N,simd::bool_mask>& test) noexcept {
    return all(test.extract_lower_half() && test.extract_upper_half());
}

/**
 * @brief Horizontal `min` of all elements in the wide vector.
 */
template <Quantity Q, std::size_t N>
inline auto hmin(const wide_vector<4,N,Q>& v) noexcept {
    auto v0 = v.template reads<0>();
    auto v1 = v.template reads<1>();
    auto v2 = v.template reads<2>();
    auto v3 = v.template reads<3>();
    // compilers should generate decent code here
    return m::min(v0,v1,v2,v3);
}
/**
 * @brief Horizontal `min` of all elements in the wide vector.
 */
template <Quantity Q, std::size_t N>
inline auto hmin(const wide_vector<8,N,Q>& v) noexcept {
    return m::min(hmin(v.extract_lower_half()), hmin(v.extract_upper_half()));
}

/**
 * @brief Horizontal `max` of all elements in the wide vector.
 */
template <Quantity Q, std::size_t N>
inline auto hmax(const wide_vector<4,N,Q>& v) noexcept {
    auto v0 = v.template reads<0>();
    auto v1 = v.template reads<1>();
    auto v2 = v.template reads<2>();
    auto v3 = v.template reads<3>();
    // compilers should generate decent code here
    return m::max(v0,v1,v2,v3);
}
/**
 * @brief Horizontal `max` of all elements in the wide vector.
 */
template <Quantity Q, std::size_t N>
inline auto hmax(const wide_vector<8,N,Q>& v) noexcept {
    return m::max(hmax(v.extract_lower_half()), hmax(v.extract_upper_half()));
}

}
