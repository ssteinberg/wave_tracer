/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/quantity/quantity_vector.hpp>
#include <wt/math/eft/eft.hpp>
#include <wt/math/unit_vector/unit_vector.hpp>

#include <wt/util/concepts.hpp>
#include <wt/math/type_traits.hpp>

namespace wt::m {

template <Vector V1, Vector V2>
constexpr inline auto dot(const V1& v1, const V2& v2) noexcept 
    requires (element_count_v<V1> == element_count_v<V2>)
{
    auto sum = v1[0]*v2[0];
    for (auto i=1ul;i<element_count_v<V1>;++i)
        sum = fma(v1[i],v2[i], sum);
    return sum;
}

template <Vector V>
constexpr inline auto length2(const V& v) noexcept {
    using T = vector_element_rep_t<V>;
    if constexpr (is_unit_vector_v<V>)
        return T{ 1 };
    return dot(v,v);
}
template <Vector V>
constexpr inline auto length(const V& v) noexcept {
    using T = vector_element_rep_t<V>;
    if constexpr (is_unit_vector_v<V>)
        return T{ 1 };
    return m::sqrt(length2(v));
}

template <Vector V>
constexpr inline auto normalize(const V& v) noexcept {
    using T = vector_element_rep_t<V>;
    constexpr auto N = element_count_v<V>;
    return unit_vector<N, T>{ v/length(v) };
}

template <Vector V1, Vector V2>
constexpr inline auto cross(const V1& x, const V2& y) noexcept 
    requires (element_count_v<V1> == 3) && (element_count_v<V2> == 3)
{
    using R = decltype(x.y*y.z - x.z*y.y);
    using V = vector3<R>;
    return V{
        eft::diff_prod(x.y,y.z, x.z,y.y),
        eft::diff_prod(x.z,y.x, x.x,y.z),
        eft::diff_prod(x.x,y.y, x.y,y.x)
    };
}


template <VectorOrUnitVector U, VectorOrUnitVector V>
constexpr inline auto outer(const U& u, const V& v) noexcept {
    constexpr auto N = element_count_v<U>;
    constexpr auto M = element_count_v<V>;
    using T = decltype(u[0]*v[0]);

    glm::mat<M,N,T> m{};
    for (auto i=0ul;i<N;++i)
    for (auto j=0ul;j<M;++j)
        m[j][i] = u[i]*v[j];
    return m;
}

}
