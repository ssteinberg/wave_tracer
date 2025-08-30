/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include "unit_vector.hpp"
#include <wt/math/quantity/quantity_vector.hpp>
#include <wt/math/eft/eft.hpp>

#include <wt/util/concepts.hpp>
#include <wt/math/type_traits.hpp>


namespace wt {

namespace m {

template <FloatingPoint T, std::size_t N>
constexpr inline auto abs(const unit_vector<N,T>& v) noexcept {
    glm::vec<N,T> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = m::abs(v[i]);
    return unit_vector<N,T>{ ret };
}


template <FloatingPoint T>
constexpr inline auto max_element(const dirvec1<T>& v) noexcept {
    return v.x;
}
template <FloatingPoint T>
constexpr inline auto max_element(const dirvec2<T>& v) noexcept {
    return max(v.x,v.y);
}
template <FloatingPoint T>
constexpr inline auto max_element(const dirvec3<T>& v) noexcept {
    return max(v.x,v.y,v.z);
}
template <FloatingPoint T>
constexpr inline auto min_element(const dirvec1<T>& v) noexcept {
    return v.x;
}
template <FloatingPoint T>
constexpr inline auto min_element(const dirvec2<T>& v) noexcept {
    return min(v.x,v.y);
}
template <FloatingPoint T>
constexpr inline auto min_element(const dirvec3<T>& v) noexcept {
    return min(v.x,v.y,v.z);
}

template <FloatingPoint T>
constexpr inline auto max_dimension(const dirvec1<T>& v) noexcept {
    return 0;
}
template <FloatingPoint T>
constexpr inline auto max_dimension(const dirvec2<T>& v) noexcept {
    return v.x>v.y ? 0 : 1;
}
template <FloatingPoint T>
constexpr inline auto max_dimension(const dirvec3<T>& v) noexcept {
    const auto e = max_element(v);
    return e==v.x ? 0 : e==v.y ? 1 : 2;
}
template <FloatingPoint T>
constexpr inline auto min_dimension(const dirvec1<T>& v) noexcept {
    return 0;
}
template <FloatingPoint T>
constexpr inline auto min_dimension(const dirvec2<T>& v) noexcept {
    return v.x<v.y ? 0 : 1;
}
template <FloatingPoint T>
constexpr inline auto min_dimension(const dirvec3<T>& v) noexcept {
    const auto e = min_element(v);
    return e==v.x ? 0 : e==v.y ? 1 : 2;
}


template <typename T>
constexpr inline auto prod(const dirvec1<T>& v) noexcept {
    return v.x;
}
template <typename T>
constexpr inline auto prod(const dirvec2<T>& v) noexcept {
    return v.x*v.y;
}
template <typename T>
constexpr inline auto prod(const dirvec3<T>& v) noexcept {
    return v.x*v.y*v.z;
}

template <FloatingPoint T>
constexpr inline auto sum(const dirvec1<T>& v) noexcept {
    return v.x;
}
template <FloatingPoint T>
constexpr inline auto sum(const dirvec2<T>& v) noexcept {
    return v.x+v.y;
}
template <FloatingPoint T>
constexpr inline auto sum(const dirvec3<T>& v) noexcept {
    return v.x+v.y+v.z;
}


}


template <FloatingPoint T, std::size_t N>
constexpr inline auto operator+(const unit_vector<N, T>& v) noexcept {
    return v;
}
template <FloatingPoint T, std::size_t N>
constexpr inline auto operator-(unit_vector<N, T> v) noexcept {
    for (auto i=0ul;i<N;++i)
        *(&v.x+i) = -*(&v.x+i);
    return v;
}


template <FloatingPoint T, Numeric S, std::size_t N>
constexpr inline auto operator*(const unit_vector<N,T>& v1, const S& s) noexcept {
    using R = decltype(std::declval<T>()*s);
    glm::vec<N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = v1[i]*s;
    return ret;
}
template <FloatingPoint T, Numeric S, std::size_t N>
constexpr inline auto operator*(const S& s, const unit_vector<N,T>& v1) noexcept {
    using R = decltype(s*std::declval<T>());
    glm::vec<N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = s*v1[i];
    return ret;
}
template <FloatingPoint T, Numeric S, std::size_t N>
constexpr inline auto operator/(const unit_vector<N,T>& v1, const S& s) noexcept {
    using R = decltype(std::declval<T>()/s);
    glm::vec<N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = v1[i]/s;
    return ret;
}
template <FloatingPoint T, Numeric S, std::size_t N>
constexpr inline auto operator/(const S& s, const unit_vector<N,T>& v1) noexcept {
    using R = decltype(s/std::declval<T>());
    glm::vec<N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = s/v1[i];
    return ret;
}


template <FloatingPoint T, Quantity Q, std::size_t N>
constexpr inline auto operator*(const unit_vector<N,T>& v1, const Q& s) noexcept {
    using R = decltype(std::declval<T>()*s);
    quantity_vector<N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = v1[i]*s;
    return ret;
}
template <FloatingPoint T, Quantity Q, std::size_t N>
constexpr inline auto operator*(const Q& s, const unit_vector<N,T>& v1) noexcept {
    using R = decltype(s*std::declval<T>());
    quantity_vector<N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = s*v1[i];
    return ret;
}
template <FloatingPoint T, Quantity Q, std::size_t N>
constexpr inline auto operator/(const unit_vector<N,T>& v1, const Q& s) noexcept {
    using R = decltype(std::declval<T>()/s);
    quantity_vector<N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = v1[i]/s;
    return ret;
}
template <FloatingPoint T, Quantity Q, std::size_t N>
constexpr inline auto operator/(const Q& s, const unit_vector<N,T>& v1) noexcept {
    using R = decltype(s/std::declval<T>());
    quantity_vector<N,R> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = s/v1[i];
    return ret;
}


template <FloatingPoint T, FloatingPoint S, std::size_t N, std::size_t M>
constexpr inline auto operator*(const mat<M,N,T>& m, const unit_vector<N,S>& v) noexcept {
    using R = decltype(std::declval<T>()*std::declval<S>());
    glm::vec<M,R> ret = {};
    for (auto j=0ul;j<M;++j)
    for (auto i=0ul;i<N;++i)
        ret[j] += m[i][j]*v[i];
    return ret;
}


template <FloatingPoint T, std::size_t N>
constexpr inline auto operator<(const unit_vector<N,T>& o1, const unit_vector<N,T>& o2) noexcept {
    bvec<N> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = o1[i]<o2[i];
    return ret;
}
template <FloatingPoint T, std::size_t N>
constexpr inline auto operator<=(const unit_vector<N,T>& o1, const unit_vector<N,T>& o2) noexcept {
    bvec<N> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = o1[i]<=o2[i];
    return ret;
}
template <FloatingPoint T, std::size_t N>
constexpr inline auto operator>(const unit_vector<N,T>& o1, const unit_vector<N,T>& o2) noexcept {
    bvec<N> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = o1[i]>o2[i];
    return ret;
}
template <FloatingPoint T, std::size_t N>
constexpr inline auto operator>=(const unit_vector<N,T>& o1, const unit_vector<N,T>& o2) noexcept {
    bvec<N> ret;
    for (auto i=0ul;i<N;++i)
        ret[i] = o1[i]>=o2[i];
    return ret;
}
template <FloatingPoint T, std::size_t N>
constexpr inline auto operator==(const unit_vector<N,T>& o1, const unit_vector<N,T>& o2) noexcept {
    for (auto i=0ul;i<N;++i)
        if (o1[i]!=o2[i])
            return false;
    return true;
}
template <FloatingPoint T, std::size_t N>
constexpr inline auto operator!=(const unit_vector<N,T>& o1, const unit_vector<N,T>& o2) noexcept {
    for (auto i=0ul;i<N;++i)
        if (o1[i]!=o2[i])
            return true;
    return false;
}

}