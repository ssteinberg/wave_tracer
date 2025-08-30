/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <bitset>

#include <wt/math/defs.hpp>
#include <wt/math/common.hpp>
#include <wt/math/quantity/defs.hpp>
#include <wt/math/quantity/zero.hpp>
#include <wt/math/quantity/quantity_vector.hpp>

#include <wt/util/concepts.hpp>

#include "common.hpp"
// SIMD mode selector
#ifdef SIMD_AVX
#include "engines/simd_avx.hpp"
#else
#include "engines/simd_emulation.hpp"
#endif


namespace wt {

namespace detail {

// SIMD mode selector
#ifdef SIMD_AVX
#define simd_t simd::simd_avx_t
#else
#define simd_t simd::simd_emulated_t
#endif

/**
 * @brief Native SIMD data type depends on the used SIMD engine.
 *        Typically should not be manipulated directly.
 */
template <FloatingPoint Fp, std::size_t Width, std::size_t N>
struct simd_data_t {};
// 4 x fp32
template <std::size_t N>
struct simd_data_t<float,4,N> {
    using simd_native_t = simd_t<float,4>;
    simd_native_t simd[N];
};
// 8 x fp32
template <std::size_t N>
struct simd_data_t<float,8,N> {
    using simd_native_t = simd_t<float,8>;
    simd_native_t simd[N];
};
// 4 x fp64
template <std::size_t N>
struct simd_data_t<double,4,N> {
    using simd_native_t = simd_t<double,4>;
    simd_native_t simd[N];
};
// 8 x fp64
template <std::size_t N>
struct simd_data_t<double,8,N> {
    using simd_native_t = simd_t<double,8>;
    simd_native_t simd[N];
};

#undef simd_t

}

namespace simd {

inline constexpr struct unaligned_data_t {} unaligned_data;
inline constexpr struct aligned_data_t {} aligned_data;

/** @brief Indicates that the elements of the wide vector are used as masks for boolean values. */
using bool_mask = quantity<u::one, std::int8_t>;
/** @brief Indicates that the elements of the wide vector are unitless. */
using unitless = quantity<u::one, f_t>;

}

/**
 * @brief Wide (4 or 8 element) floating point vectors, each vector of `N` elements (up to 4). Used SIMD type and instruction set depends on `wt::f_t`, designed to support 32-bit and 64-bit precision. The resulting data type is of `Width` x `N` element count.
 *        Supports quantities.
 *
 * @tparam Width width (count of simd elements) of the vector, must be 4 or 8
 * @tparam N number of vector elements (N==1 is treated as a scalar), must be 1<=N<=4
 * @tparam Q quantity of the vector (`simd::unitless` indicates unitless, `simd::bool_mask` indicates boolean masks)
 */
template <std::size_t Width, std::size_t N, Quantity Q>
struct wide_vector : private detail::simd_data_t<f_t, Width, N> {
    using Fp = f_t;

    static constexpr auto is_bool_mask = std::is_same_v<Q, simd::bool_mask>;

    static_assert(std::is_same_v<typename Q::rep,f_t> || is_bool_mask);
    static_assert(std::is_same_v<Fp, float> || std::is_same_v<Fp, double>,
                  "only quantities with float or double representation can be used");

    static_assert(Width==4 || Width==8, "width of SIMD vector must be 4 or 8");
    static_assert(1<=N && N<=4, "vertical size of the vector must be between 1 and 4");

    static constexpr auto width = Width;
    static constexpr bool is_scalar = N==1;

    static constexpr auto unit = Q::unit;
    static constexpr bool is_unitless = unit == u::one;

    static_assert(is_unitless || !is_bool_mask);

    /**
     * @brief Type of a single element of the wide vector -- scalar of quantity `Q`.
     */
    using S = std::conditional_t<is_unitless, Fp, Q>;
    /**
     * @brief Type of a horizontal slice of the wide vector -- `N` element vector of quantity `Q`.
     */
    using T = std::conditional_t<
        is_scalar,
        S,
        std::conditional_t<is_unitless, vec<N, Fp>, qvec<N, Q>>
    >;

    using simd_native_t = detail::simd_data_t<f_t, Width, N>::simd_native_t;

private:
    static constexpr inline auto to_rep(S t) noexcept requires(is_unitless) {
        return t;
    }
    static constexpr inline auto to_rep(S q) noexcept requires(!is_unitless) {
        return q.numerical_value_in(unit);
    }
    static constexpr inline auto to_q(Fp t) noexcept requires(is_unitless) {
        return t;
    }
    static constexpr inline auto to_q(Fp t) noexcept requires(!is_unitless) {
        return t * unit;
    }

public:
    wide_vector() noexcept = default;
    inline wide_vector(const wide_vector& o) noexcept = default;

    /**
     * @brief Construct a scalar wide vector from a native SIMD wide scalar. Typically should not be used.
     */
    inline explicit wide_vector(const simd_native_t& v) noexcept requires (N==1) {
        this->simd[0] = v;
    }
    /**
     * @brief Construct a wide vector from native SIMD wide scalars. Typically should not be used.
     */
    inline explicit wide_vector(const simd_native_t& x,
                                const simd_native_t& y) noexcept requires (N==2) {
        this->simd[0] = x;
        this->simd[1] = y;
    }
    /**
     * @brief Construct a wide vector from native SIMD wide scalars. Typically should not be used.
     */
    inline explicit wide_vector(const simd_native_t& x,
                                const simd_native_t& y,
                                const simd_native_t& z) noexcept requires (N==3) {
        this->simd[0] = x;
        this->simd[1] = y;
        this->simd[2] = z;
    }
    /**
     * @brief Construct a wide vector from native SIMD wide scalars. Typically should not be used.
     */
    inline explicit wide_vector(const simd_native_t& x,
                                const simd_native_t& y,
                                const simd_native_t& z,
                                const simd_native_t& w) noexcept requires (N==4) {
        this->simd[0] = x;
        this->simd[1] = y;
        this->simd[2] = z;
        this->simd[3] = w;
    }

    /**
     * @brief Access the underlying SIMD element for vector index `n`. Typically, should not be used.
     */
    inline auto& simd_native(std::size_t n) noexcept {
        assert(n<Width);
        return this->simd[n];
    }
    /**
     * @brief Access the underlying SIMD element for vector index `n`. Typically, should not be used.
     */
    inline const auto& simd_native(std::size_t n) const noexcept {
        assert(n<Width);
        return this->simd[n];
    }

    /**
     * @brief Construct a scalar wide vector from a single scalar `scalar`, copying `scalar` into all wide elements.
     */
    template <typename T2>
        requires std::is_constructible_v<T, T2>
    inline explicit wide_vector(const T2& scalar) noexcept requires (N==1) {
        simd::set1(this->simd[0], to_rep(T{ scalar }));
    }
    /**
     * @brief Construct a wide vector from a single vector `vec`, copying `vec` into all wide elements.
     */
    template <typename T2>
        requires std::is_constructible_v<T, T2>
    inline explicit wide_vector(const T2& vec) noexcept requires (N>1) {
        const auto t = T{ vec };
        for (std::size_t n=0;n<N;++n)
            simd::set1(simd_native(n), to_rep(t[n]));
    }

    /**
     * @brief Explicit construction of a bool mask from a unitless wide vector
     */
    inline explicit wide_vector(const wide_vector<Width,N,simd::unitless>& v) noexcept requires (is_bool_mask) {
        for (std::size_t n=0;n<N;++n)
            simd_native(n) = v.simd_native(n);
    }

    /**
     * @brief Construct an 8-wide vector from 2 4-wide vectors.
     */
    inline explicit wide_vector(wide_vector<4,N,Q> lower,
                                wide_vector<4,N,Q> upper) noexcept requires (N==1 && Width==8) {
        simd_native(0) = simd::merge_lower_upper(lower.simd_native(0), upper.simd_native(0));
    }
    /**
     * @brief Construct an 8-wide vector from 2 4-wide vectors.
     */
    inline explicit wide_vector(wide_vector<4,N,Q> lower,
                                wide_vector<4,N,Q> upper) noexcept requires (N>1 && Width==8) {
        for (std::size_t n=0;n<N;++n)
            simd_native(n) = simd::merge_lower_upper(lower.simd_native(n), upper.simd_native(n));
    }

    /**
     * @brief Construct a wide vector from scalar wide vectors.
     */
    inline explicit wide_vector(wide_vector<Width,1,Q> x,
                                wide_vector<Width,1,Q> y) noexcept requires (N==2) {
        simd_native(0) = x.simd_native(0);
        simd_native(1) = y.simd_native(0);
    }
    /**
     * @brief Construct a wide vector from scalar wide vectors.
     */
    inline explicit wide_vector(wide_vector<Width,1,Q> x,
                                wide_vector<Width,1,Q> y,
                                wide_vector<Width,1,Q> z) noexcept requires (N==3) {
        simd_native(0) = x.simd_native(0);
        simd_native(1) = y.simd_native(0);
        simd_native(2) = z.simd_native(0);
    }
    /**
     * @brief Construct a wide vector from scalar wide vectors.
     */
    inline explicit wide_vector(wide_vector<Width,1,Q> x,
                                wide_vector<Width,1,Q> y,
                                wide_vector<Width,1,Q> z,
                                wide_vector<Width,1,Q> w) noexcept requires (N==4) {
        simd_native(0) = x.simd_native(0);
        simd_native(1) = y.simd_native(0);
        simd_native(2) = z.simd_native(0);
        simd_native(3) = w.simd_native(0);
    }

    /**
     * @brief Construct a scalar wide vector by explicitly setting each wide element.
     */
    inline explicit wide_vector(const T& w0,
                                const T& w1,
                                const T& w2,
                                const T& w3) noexcept requires (N==1 && Width==4) {
        simd::set(this->simd[0], { to_rep(w0),to_rep(w1),to_rep(w2),to_rep(w3) });
    }
    /**
     * @brief Construct a scalar wide vector by explicitly setting each wide element.
     */
    inline explicit wide_vector(const T& w0,
                                const T& w1,
                                const T& w2,
                                const T& w3,
                                const T& w4,
                                const T& w5,
                                const T& w6,
                                const T& w7) noexcept requires (N==1 && Width==8) {
        simd::set(this->simd[0], {
            to_rep(w0),to_rep(w1),to_rep(w2),to_rep(w3),
            to_rep(w4),to_rep(w5),to_rep(w6),to_rep(w7)
        });
    }
    /**
     * @brief Construct a wide vector by explicitly setting each wide element.
     */
    inline explicit wide_vector(const T& w0,
                                const T& w1,
                                const T& w2,
                                const T& w3) noexcept requires (N>1 && Width==4) {
        for (std::size_t n=0;n<N;++n)
            simd::set(this->simd[n], { to_rep(w0[n]),to_rep(w1[n]),to_rep(w2[n]),to_rep(w3[n]) });
    }
    /**
     * @brief Construct a wide vector by explicitly setting each wide element.
     */
    inline explicit wide_vector(const T& w0,
                                const T& w1,
                                const T& w2,
                                const T& w3,
                                const T& w4,
                                const T& w5,
                                const T& w6,
                                const T& w7) noexcept requires (N>1 && Width==8) {
        for (std::size_t n=0;n<N;++n)
            simd::set(this->simd[n], {
                to_rep(w0[n]),to_rep(w1[n]),to_rep(w2[n]),to_rep(w3[n]),
                to_rep(w4[n]),to_rep(w5[n]),to_rep(w6[n]),to_rep(w7[n])
            });
    }

    /**
     * @brief Loads wide data from arbitrarily-aligned address.
     */
    inline explicit wide_vector(const S* data,
                                simd::unaligned_data_t) noexcept requires (N==1) {
        simd::loadu(this->simd[0], (const Fp*)data);
    }
    /**
     * @brief Loads wide data from arbitrarily-aligned address.
     */
    inline explicit wide_vector(const S* x, const S* y,
                                simd::unaligned_data_t) noexcept requires (N==2) {
        simd::loadu(this->simd[0], (const Fp*)x);
        simd::loadu(this->simd[1], (const Fp*)y);
    }
    /**
     * @brief Loads wide data from arbitrarily-aligned address.
     */
    inline explicit wide_vector(const S* x, const S* y, const S* z,
                                simd::unaligned_data_t) noexcept requires (N==3) {
        simd::loadu(this->simd[0], (const Fp*)x);
        simd::loadu(this->simd[1], (const Fp*)y);
        simd::loadu(this->simd[2], (const Fp*)z);
    }
    /**
     * @brief Loads wide data from arbitrarily-aligned address.
     */
    inline explicit wide_vector(const S* x, const S* y, const S* z, const S* w,
                                simd::unaligned_data_t) noexcept requires (N==4) {
        simd::loadu(this->simd[0], (const Fp*)x);
        simd::loadu(this->simd[1], (const Fp*)y);
        simd::loadu(this->simd[2], (const Fp*)z);
        simd::loadu(this->simd[3], (const Fp*)w);
    }
    /**
     * @brief Loads wide data from address, assumes address is aligned to `sizeof(Fp)*Width` boundary.
     */
    inline explicit wide_vector(const S* data,
                                simd::aligned_data_t) noexcept requires (N==1) {
        simd::load(this->simd[0], (const Fp*)data);
    }
    /**
     * @brief Loads wide data from address, assumes address is aligned to `sizeof(Fp)*Width` boundary.
     */
    inline explicit wide_vector(const S* x, const S* y,
                                simd::aligned_data_t) noexcept requires (N==2) {
        simd::load(this->simd[0], (const Fp*)x);
        simd::load(this->simd[1], (const Fp*)y);
    }
    /**
     * @brief Loads wide data from address, assumes address is aligned to `sizeof(Fp)*Width` boundary.
     */
    inline explicit wide_vector(const S* x, const S* y, const S* z,
                                simd::aligned_data_t) noexcept requires (N==3) {
        simd::load(this->simd[0], (const Fp*)x);
        simd::load(this->simd[1], (const Fp*)y);
        simd::load(this->simd[2], (const Fp*)z);
    }
    /**
     * @brief Loads wide data from address, assumes address is aligned to `sizeof(Fp)*Width` boundary.
     */
    inline explicit wide_vector(const S* x, const S* y, const S* z, const S* w,
                                simd::aligned_data_t) noexcept requires (N==3) {
        simd::load(this->simd[0], (const Fp*)x);
        simd::load(this->simd[1], (const Fp*)y);
        simd::load(this->simd[2], (const Fp*)z);
        simd::load(this->simd[3], (const Fp*)w);
    }

    inline explicit wide_vector(const wide_vector<4,N,Q>& o) noexcept
        requires (Width==8) && (std::is_same_v<Fp,float>)
    {
        for (std::size_t n=0;n<N;++n)
            simd_native(n) = simd::cast_to_256(o.simd_native(n));
    }
    inline explicit wide_vector(const wide_vector<4,N,Q>& o) noexcept
        requires (Width==8) && (std::is_same_v<Fp,double>)
    {
        for (std::size_t n=0;n<N;++n)
            simd_native(n) = simd::cast_to_512d(o.simd_native(n));
    }

    inline wide_vector& operator=(const wide_vector& o) noexcept = default;

    /**
     * @brief Extracts a horizontal slice as a scalar wide vector from element at position `n` (between 0 and `N-1`) in the vector.
     */
    inline auto operator[](std::size_t n) const noexcept {
        wide_vector<Width,1,Q> v;
        v.simd_native(0) = simd_native(n);
        return v;
    }
    /**
     * @brief Extracts the `n==0` scalar wide vector, equivalent to `operator[0]`.
     */
    inline auto x() const noexcept                { return (*this)[0]; }
    /**
     * @brief Extracts the `n==1` scalar wide vector, equivalent to `operator[1]`.
     */
    inline auto y() const noexcept requires (N>1) { return (*this)[1]; }
    /**
     * @brief Extracts the `n==2` scalar wide vector, equivalent to `operator[2]`.
     */
    inline auto z() const noexcept requires (N>2) { return (*this)[2]; }
    /**
     * @brief Extracts the `n==3` scalar wide vector, equivalent to `operator[3]`.
     */
    inline auto w() const noexcept requires (N>3) { return (*this)[3]; }

    /**
     * @brief Casts an 8-wide vector into a 4-wide vector by extracting the 4 lower wide elements.
     */
    inline auto extract_lower_half() const noexcept requires (Width==8) {
        wide_vector<4,N,Q> ret;
        for (std::size_t n=0;n<N;++n)
            ret.simd_native(n) = simd::extract_lower_half(simd_native(n));
        return ret;
    }
    /**
     * @brief Casts an 8-wide vector into a 4-wide vector by extracting the 4 upper wide elements.
     */
    inline auto extract_upper_half() const noexcept requires (Width==8) {
        wide_vector<4,N,Q> ret;
        for (std::size_t n=0;n<N;++n)
            ret.simd_native(n) = simd::extract_upper_half(simd_native(n));
        return ret;
    }

    /**
     * @brief Returns a vertical slice of the wide vector at position `w` between 0 and `Width-1`.
     * @tparam idx the compile-time known wide index
     */
    template <std::size_t idx>
    inline auto reads() const noexcept requires (N==1)
    {
        return to_q(simd_native(0).template extract_static<idx>());
    }
    /**
     * @brief Returns a vertical slice of the wide vector at position `w` between 0 and `Width-1`.
     * @tparam idx the compile-time known wide index
     */
    template <std::size_t idx>
    inline auto reads() const noexcept requires (is_unitless && N>1)
    {
        vec<N,S> v;
        for (std::size_t n=0;n<N;++n)
            v[n] = to_q(simd_native(n).template extract_static<idx>());
        return v;
    }
    /**
     * @brief Returns a vertical slice of the wide vector at position `w` between 0 and `Width-1`.
     * @tparam idx the compile-time known wide index
     */
    template <std::size_t idx>
    inline auto reads() const noexcept requires (!is_unitless && N>1)
    {
        qvec<N,S> v;
        for (std::size_t n=0;n<N;++n)
            v[n] = to_q(simd_native(n).template extract_static<idx>());
        return v;
    }

    /**
     * @brief Returns a vertical slice of the wide vector at position `w` between 0 and `Width-1`. Dynamic index, slower than `reads`.
     */
    inline auto read(std::size_t w) const noexcept requires (N==1)
    {
        return to_q(simd_native(0).extract(w));
    }
    /**
     * @brief Returns a vertical slice of the wide vector at position `w` between 0 and `Width-1`. Dynamic index, slower than `reads`.
     */
    inline auto read(std::size_t w) const noexcept requires (is_unitless && N>1)
    {
        vec<N,S> v;
        for (std::size_t n=0;n<N;++n)
            v[n] = to_q(simd_native(n).extract(w));
        return v;
    }
    /**
     * @brief Returns a vertical slice of the wide vector at position `w` between 0 and `Width-1`. Dynamic index, slower than `reads`.
     */
    inline auto read(std::size_t w) const noexcept requires (!is_unitless && N>1)
    {
        qvec<N,S> v;
        for (std::size_t n=0;n<N;++n)
            v[n] = to_q(simd_native(n).extract(w));
        return v;
    }

    inline wide_vector& operator+=(const wide_vector& o) noexcept {
        for (std::size_t n=0;n<N;++n)
            simd_native(n) = simd::add(simd_native(n), o.simd_native(n));
        return *this;
    }
    inline wide_vector& operator-=(const wide_vector& o) noexcept {
        for (std::size_t n=0;n<N;++n)
            simd_native(n) = simd::sub(simd_native(n), o.simd_native(n));
        return *this;
    }
    inline wide_vector& operator*=(const wide_vector& o) noexcept requires(is_unitless) {
        for (std::size_t n=0;n<N;++n)
            simd_native(n) = simd::mul(simd_native(n), o.simd_native(n));
        return *this;
    }
    inline wide_vector& operator/=(const wide_vector& o) noexcept requires(is_unitless) {
        for (std::size_t n=0;n<N;++n)
            simd_native(n) = simd::div(simd_native(n), o.simd_native(n));
        return *this;
    }
    inline wide_vector& operator*=(const Fp& t) noexcept {
        const auto s = wide_vector<Width,N,simd::unitless>{ t };
        for (std::size_t n=0;n<N;++n)
            simd_native(n) = simd::mul(simd_native(n), s);
        return *this;
    }
    inline wide_vector& operator/=(const Fp& t) noexcept {
        const auto s = wide_vector<Width,N,simd::unitless>{ t };
        for (std::size_t n=0;n<N;++n)
            simd_native(n) = simd::div(simd_native(n), s);
        return *this;
    }

    /**
     * @brief Logical AND.
     */
    inline wide_vector& operator&=(const wide_vector& o) noexcept requires (is_bool_mask) {
        for (auto i=0ul;i<N;++i)
            this->simd_native(i) = simd::land(this->simd_native(i), o.simd_native(i));
        return *this;
    }
    /**
     * @brief Logical OR.
     */
    inline wide_vector& operator|=(const wide_vector& o) noexcept requires (is_bool_mask) {
        for (auto i=0ul;i<N;++i)
            this->simd_native(i) = simd::lor(this->simd_native(i), o.simd_native(i));
        return *this;
    }

    /**
     * @brief Logical (bool masks) equality comparison.
     */
    inline wide_vector<Width,N,simd::bool_mask> operator==(const wide_vector& o) const noexcept
        requires(is_bool_mask)
    {
        wide_vector<Width,N,simd::bool_mask> ret;
        for (std::size_t n=0;n<N;++n)
            ret.simd_native(n) = simd::leq(simd_native(n), o.simd_native(n));
        return ret;
    }
    /**
     * @brief Logical (bool masks) inequality comparison.
     */
    inline wide_vector<Width,N,simd::bool_mask> operator!=(const wide_vector& o) const noexcept
        requires(is_bool_mask)
    {
        wide_vector<Width,N,simd::bool_mask> ret;
        for (std::size_t n=0;n<N;++n)
            ret.simd_native(n) = simd::lneq(simd_native(n), o.simd_native(n));
        return ret;
    }

    /**
     * @brief Floating point equality comparison.
     */
    inline wide_vector<Width,N,simd::bool_mask> operator==(const wide_vector& o) const noexcept
        requires(!is_bool_mask)
    {
        wide_vector<Width,N,simd::bool_mask> ret;
        for (std::size_t n=0;n<N;++n)
            ret.simd_native(n) = simd::eq(simd_native(n), o.simd_native(n));
        return ret;
    }
    /**
     * @brief Floating point inequality comparison.
     */
    inline wide_vector<Width,N,simd::bool_mask> operator!=(const wide_vector& o) const noexcept
        requires(!is_bool_mask)
    {
        wide_vector<Width,N,simd::bool_mask> ret;
        for (std::size_t n=0;n<N;++n)
            ret.simd_native(n) = simd::neq(simd_native(n), o.simd_native(n));
        return ret;
    }
    inline wide_vector<Width,N,simd::bool_mask> operator<(const wide_vector& o) const noexcept
        requires(!is_bool_mask)
    {
        wide_vector<Width,N,simd::bool_mask> ret;
        for (std::size_t n=0;n<N;++n)
            ret.simd_native(n) = simd::lt(simd_native(n), o.simd_native(n));
        return ret;
    }
    inline wide_vector<Width,N,simd::bool_mask> operator>(const wide_vector& o) const noexcept
        requires(!is_bool_mask)
    {
        wide_vector<Width,N,simd::bool_mask> ret;
        for (std::size_t n=0;n<N;++n)
            ret.simd_native(n) = simd::gt(simd_native(n), o.simd_native(n));
        return ret;
    }
    inline wide_vector<Width,N,simd::bool_mask> operator<=(const wide_vector& o) const noexcept
        requires(!is_bool_mask)
    {
        wide_vector<Width,N,simd::bool_mask> ret;
        for (std::size_t n=0;n<N;++n)
            ret.simd_native(n) = simd::le(simd_native(n), o.simd_native(n));
        return ret;
    }
    inline wide_vector<Width,N,simd::bool_mask> operator>=(const wide_vector& o) const noexcept
        requires(!is_bool_mask)
    {
        wide_vector<Width,N,simd::bool_mask> ret;
        for (std::size_t n=0;n<N;++n)
            ret.simd_native(n) = simd::ge(simd_native(n), o.simd_native(n));
        return ret;
    }

    inline wide_vector<Width,N,simd::bool_mask> operator==(zero_t) const noexcept {
        return *this == this->zero();
    }
    inline wide_vector<Width,N,simd::bool_mask> operator!=(zero_t) const noexcept {
        return *this != this->zero();
    }
    inline wide_vector<Width,N,simd::bool_mask> operator<(zero_t) const noexcept {
        return *this < this->zero();
    }
    inline wide_vector<Width,N,simd::bool_mask> operator>(zero_t) const noexcept {
        return *this > this->zero();
    }
    inline wide_vector<Width,N,simd::bool_mask> operator<=(zero_t) const noexcept {
        return *this <= this->zero();
    }
    inline wide_vector<Width,N,simd::bool_mask> operator>=(zero_t) const noexcept {
        return *this >= this->zero();
    }

    /**
     * @brief Converts a unitless wide vector to a vector of `std::bitset`.
     *        This is useful for reading the results of comparisons operators.
     */
    inline auto to_bitmask() const noexcept requires(is_bool_mask && is_scalar) {
        using B = std::bitset<Width>;
        B mask;
        for (auto i=0;i<Width;++i)
            mask.set(i, m::signbit(this->read(i)));
        return mask;
    }
    /**
     * @brief Converts a unitless wide vector to a vector of `std::bitset`.
     *        This is useful for reading the results of comparison operators.
     */
    inline auto to_bitmask() const noexcept requires(is_bool_mask && !is_scalar) {
        using B = std::bitset<Width>;
        std::array<B, N> mask;
        for (auto i=0;i<Width;++i) {
            for (auto n=0;n<N;++n)
                mask[n].set(i, m::signbit(this->read(i)[n]));
        }
        return mask;
    }

    /**
     * @brief Quantity cast. This may be a lossy cast.
     */
    template <std::convertible_to<Q> Q2>
    inline operator wide_vector<Width,N,Q2>() const noexcept requires (!is_bool_mask) {
        wide_vector<Width,N,Q2> ret;

        constexpr auto scale = (Fp(1)*Q::unit).numerical_value_in(Q2::unit);
        if (scale != 1) {
            const auto wscale = wide_vector<Width,1,simd::unitless>::from_scalar(scale).simd_native(0);
            for (auto i=0ul;i<N;++i)
                ret.simd_native(i) = simd::mul(simd_native(i), wscale);
        } else {
            for (auto i=0ul;i<N;++i)
                ret.simd_native(i) = simd_native(i);
        }

        return ret;
    }
    /**
     * @brief Cast a scalar wide vector to a wide vector. Also does a quantity cast which may be a lossy cast.
     */
    template <std::convertible_to<Q> Q2, std::size_t N2>
    inline explicit operator wide_vector<Width,N2,Q2>() const noexcept requires (N==1 && N2>1 && !is_bool_mask) {
        // scalar -> vector
        wide_vector<Width,N2,Q> temp;
        for (auto i=0ul;i<N2;++i)
            temp.simd_native(i) = simd_native(0);
        if constexpr (std::is_same_v<Q,Q2>)
            return temp;
        // and quantity cast
        return static_cast<wide_vector<Width,N2,Q2>>(temp);
    }

    /**
     * @brief Constructs from a single scalar, i.e. sets all `Width*N` elements to `scalar`.
     */
    template <typename T2>
        requires std::is_constructible_v<T, T2>
    static inline wide_vector from_scalar(T2 scalar) noexcept requires (N==1) {
        return wide_vector{ T{ scalar } };
    }
    /**
     * @brief Constructs from a single vector, i.e. sets all `Width*N` elements to `scalar`.
     */
    template <typename S2>
        requires std::is_constructible_v<S, S2>
    static inline wide_vector from_vector(S2 scalar) noexcept requires (N>1) {
        wide_vector ret;
        for (std::size_t i=0;i<N;++i)
            simd::set1(ret.simd_native(i), to_rep(S{ scalar }));
        return ret;
    }

    static inline wide_vector zero() noexcept requires(is_unitless && !is_bool_mask) {
        return wide_vector::from_scalar(Fp(0));
    }
    static inline wide_vector zero() noexcept requires(!is_unitless && !is_bool_mask) {
        return wide_vector::from_scalar(S::zero());
    }

    static inline wide_vector one() noexcept requires(is_unitless && !is_bool_mask) {
        return wide_vector::from_scalar(Fp(1));
    }

    static inline wide_vector inf() noexcept requires(is_unitless && !is_bool_mask) {
        return wide_vector::from_scalar(limits<Fp>::infinity());
    }
    static inline wide_vector inf() noexcept requires(!is_unitless && !is_bool_mask) {
        return wide_vector::from_scalar(limits<S>::infinity());
    }

    /**
     * @brief Creates a unitless wide vector with all its elements initialized to a mask interpreted as TRUE.
     */
    static inline wide_vector mask_true() noexcept requires(is_bool_mask) {
        auto mask = simd::scalar_logical_true_value<Fp>{}();
        return wide_vector::from_scalar(mask);
    }
    /**
     * @brief Creates a unitless wide vector with all its elements initialized to 0, also interpreted as FALSE.
     */
    static inline wide_vector mask_false() noexcept requires(is_bool_mask) {
        return wide_vector::zero();
    }
};


/**
 * @brief Wide SIMD scalar bool masks.
 */
template <std::size_t W>
using b_w_t = wide_vector<W, 1, simd::bool_mask>;
/**
 * @brief 4-wide SIMD scalar bool masks.
 */
using b_w4_t = b_w_t<4>;
/**
 * @brief 8-wide SIMD scalar bool masks.
 */
using b_w8_t = b_w_t<8>;

/**
 * @brief Wide SIMD vector bool masks.
 */
template <std::size_t W>
using bvec2_w_t = wide_vector<W, 2, simd::bool_mask>;
/**
 * @brief 4-wide SIMD vector bool masks.
 */
using bvec2_w4_t = bvec2_w_t<4>;
/**
 * @brief 8-wide SIMD vector bool masks.
 */
using bvec2_w8_t = bvec2_w_t<8>;

/**
 * @brief Wide SIMD vector bool masks.
 */
template <std::size_t W>
using bvec3_w_t = wide_vector<W, 3, simd::bool_mask>;
/**
 * @brief 4-wide SIMD vector bool masks.
 */
using bvec3_w4_t = bvec3_w_t<4>;
/**
 * @brief 8-wide SIMD vector bool masks.
 */
using bvec3_w8_t = bvec3_w_t<8>;

/**
 * @brief Wide SIMD vector bool masks.
 */
template <std::size_t W>
using bvec4_w_t = wide_vector<W, 4, simd::bool_mask>;
/**
 * @brief 4-wide SIMD vector bool masks.
 */
using bvec4_w4_t = bvec4_w_t<4>;
/**
 * @brief 8-wide SIMD vector bool masks.
 */
using bvec4_w8_t = bvec4_w_t<8>;

/**
 * @brief Wide SIMD scalar floating point `f_t`.
 */
template <std::size_t W>
using f_w_t = wide_vector<W, 1, simd::unitless>;
/**
 * @brief 4-wide SIMD scalar floating point `f_t`.
 */
using f_w4_t = f_w_t<4>;
/**
 * @brief 8-wide SIMD scalar floating point `f_t`.
 */
using f_w8_t = f_w_t<8>;

/**
 * @brief Wide SIMD scalar quantity.
 */
template <std::size_t W, Quantity Q>
using q_w_t = wide_vector<W, 1, Q>;
/**
 * @brief 4-wide SIMD scalar quantity.
 */
template <Quantity Q>
using q_w4_t = q_w_t<4,Q>;
/**
 * @brief 8-wide SIMD scalar quantity.
 */
template <Quantity Q>
using q_w8_t = q_w_t<8,Q>;

/**
 * @brief Wide SIMD scalar position (units of metre).
 */
template <std::size_t W>
using length_w_t = q_w_t<W, length_t>;
/**
 * @brief 4-wide SIMD scalar position (units of metre).
 */
using length_w4_t = length_w_t<4>;
/**
 * @brief 8-wide SIMD scalar position (units of metre).
 */
using length_w8_t = length_w_t<8>;

/**
 * @brief Wide SIMD 2-element vector.
 */
template <std::size_t W>
using vec2_w_t = wide_vector<W, 2, simd::unitless>;
/**
 * @brief 4-wide SIMD 2-element vector.
 */
using vec2_w4_t = vec2_w_t<4>;
/**
 * @brief 8-wide SIMD 2-element vector.
 */
using vec2_w8_t = vec2_w_t<8>;

/**
 * @brief Wide SIMD 3-element vector.
 */
template <std::size_t W>
using vec3_w_t = wide_vector<W, 3, simd::unitless>;
/**
 * @brief 4-wide SIMD 3-element vector.
 */
using vec3_w4_t = vec3_w_t<4>;
/**
 * @brief 8-wide SIMD 3-element vector.
 */
using vec3_w8_t = vec3_w_t<8>;

/**
 * @brief Wide SIMD 4-element vector.
 */
template <std::size_t W>
using vec4_w_t = wide_vector<W, 4, simd::unitless>;
/**
 * @brief 4-wide SIMD 4-element vector.
 */
using vec4_w4_t = vec4_w_t<4>;
/**
 * @brief 8-wide SIMD 4-element vector.
 */
using vec4_w8_t = vec4_w_t<8>;

/**
 * @brief Wide SIMD 4-element vector of quantity `Q`.
 */
template <std::size_t W, Quantity Q>
using qvec2_w_t = wide_vector<W, 2, Q>;
/**
 * @brief 4-wide SIMD 2-element vector of quantity `Q`.
 */
template <Quantity Q>
using qvec2_w4_t = qvec2_w_t<4,Q>;
/**
 * @brief 8-wide SIMD 2-element vector of quantity `Q`.
 */
template <Quantity Q>
using qvec2_w8_t = qvec2_w_t<8,Q>;

/**
 * @brief Wide SIMD 4-element vector of quantity `Q`.
 */
template <std::size_t W, Quantity Q>
using qvec3_w_t = wide_vector<W, 3, Q>;
/**
 * @brief 4-wide SIMD 3-element vector of quantity `Q`.
 */
template <Quantity Q>
using qvec3_w4_t = qvec3_w_t<4,Q>;
/**
 * @brief 8-wide SIMD 3-element vector of quantity `Q`.
 */
template <Quantity Q>
using qvec3_w8_t = qvec3_w_t<8,Q>;

/**
 * @brief Wide SIMD 4-element vector of quantity `Q`.
 */
template <std::size_t W, Quantity Q>
using qvec4_w_t = wide_vector<W, 4, Q>;
/**
 * @brief 4-wide SIMD 4-element vector of quantity `Q`.
 */
template <Quantity Q>
using qvec4_w4_t = qvec4_w_t<4,Q>;
/**
 * @brief 8-wide SIMD 4-element vector of quantity `Q`.
 */
template <Quantity Q>
using qvec4_w8_t = qvec4_w_t<8,Q>;

/**
 * @brief Wide SIMD 2-element position vector (units of metre).
 */
template <std::size_t W>
using pqvec2_w_t = qvec2_w_t<W, length_t>;
/**
 * @brief 4-wide SIMD 2-element position vector (units of metre).
 */
using pqvec2_w4_t = pqvec2_w_t<4>;
/**
 * @brief 8-wide SIMD 2-element position vector (units of metre).
 */
using pqvec2_w8_t = pqvec2_w_t<8>;

/**
 * @brief Wide SIMD 3-element position vector (units of metre).
 */
template <std::size_t W>
using pqvec3_w_t = qvec3_w_t<W, length_t>;
/**
 * @brief 4-wide SIMD 3-element position vector (units of metre).
 */
using pqvec3_w4_t = pqvec3_w_t<4>;
/**
 * @brief 8-wide SIMD 3-element position vector (units of metre).
 */
using pqvec3_w8_t = pqvec3_w_t<8>;

/**
 * @brief Wide SIMD 4-element position vector (units of metre).
 */
template <std::size_t W>
using pqvec4_w_t = qvec4_w_t<W, length_t>;
/**
 * @brief 4-wide SIMD 4-element position vector (units of metre).
 */
using pqvec4_w4_t = pqvec4_w_t<4>;
/**
 * @brief 8-wide SIMD 4-element position vector (units of metre).
 */
using pqvec4_w8_t = pqvec4_w_t<8>;


}


#include "math.hpp"

