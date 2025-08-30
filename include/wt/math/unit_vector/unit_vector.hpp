/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cstddef>

#include <wt/util/unreachable.hpp>
#include <wt/util/assert.hpp>

#include <wt/util/concepts.hpp>

#include <wt/math/defs.hpp>
#include <wt/math/quantity/quantity_vector.hpp>

namespace wt {

template <std::size_t N, FloatingPoint Q>
struct unit_vector_storage;

template <FloatingPoint T>
struct unit_vector_storage<1,T> { T x; };
template <FloatingPoint T>
struct unit_vector_storage<2,T> { T x,y; };
template <FloatingPoint T>
struct unit_vector_storage<3,T> { T x,y,z; };

template <std::size_t N, FloatingPoint T>
    requires (N>=1 && N<=3)
struct unit_vector final : public unit_vector_storage<N, T> {
public:
    static constexpr auto element_count = N;
    using element_type = T;

private:
    inline T check_unit_vector_runtime(const glm::vec<N,T>& v) const {
        // allow some tolerance, letting implementation avoid extensive renormalizations
        static constexpr T check_unit_vector_tolerance_scale = 5;
        assert_unit_vector<T>(v, check_unit_vector_tolerance_scale);
        return v[0];
    }
    // consteval inline T check_unit_vector_static(const glm::vec<N,T>& v) const {
    //     double norm;
    //     if constexpr (N==1) norm = double(v.x)*v.x;
    //     if constexpr (N==2) norm = double(v.x)*v.x + double(v.y)*v.y;
    //     if constexpr (N==3) norm = double(v.x)*v.x + double(v.y)*v.y + double(v.z)*v.z;

    //     int checker = std::abs(1-norm)<1e-10 ? 1 : 0;
    //     // this will fail to compile when norm is not ~1
    //     return v[0] / (checker==1);         // unit_vector initialized with a non-unit vector!
    // }
    constexpr inline T check_unit_vector(const glm::vec<N,T>& v) const {
        // if consteval {
        //     return check_unit_vector_static(v);
        // } else {
            return check_unit_vector_runtime(v);
        // }
    }

public:
    constexpr unit_vector() noexcept = delete;

    constexpr inline unit_vector(T x) requires (N==1)
    {
        this->x=x;
        check_unit_vector({ x });
    }

    constexpr inline unit_vector(T x, T y) requires (N==2)
    {
        this->x=x;
        this->y=y;
        check_unit_vector({ x,y });
    }

    constexpr inline unit_vector(T x, T y, T z) requires (N==3)
    {
        this->x=x;
        this->y=y;
        this->z=z;
        check_unit_vector({ x,y,z });
    }
    constexpr inline unit_vector(const unit_vector<2, T>& xy, T z) requires (N==3)
    {
        this->x=xy.x;
        this->y=xy.y;
        this->z=z;
        check_unit_vector({ xy.x,xy.y,z });
    }
    constexpr inline unit_vector(const glm::vec<2, T>& xy, T z) requires (N==3)
    {
        this->x=xy.x;
        this->y=xy.y;
        this->z=z;
        check_unit_vector({ xy.x,xy.y,z });
    }
    constexpr inline unit_vector(T x, const unit_vector<2, T>& yz) requires (N==3)
    {
        this->x=x;
        this->y=yz.x;
        this->z=yz.y;
        check_unit_vector({ x,yz.x,yz.y });
    }
    constexpr inline unit_vector(T x, const glm::vec<2, T>& yz) requires (N==3)
    {
        this->x=x;
        this->y=yz.x;
        this->z=yz.y;
        check_unit_vector({ x,yz.x,yz.y });
    }

    template <FloatingPoint S>
        requires std::constructible_from<T, S>
    explicit constexpr inline unit_vector(const vec<N, S>& v) {
        this->x = T{ v[0] };
        if constexpr (N>=2) this->y = T{ v[1] };
        if constexpr (N>=3) this->z = T{ v[2] };
        
        check_unit_vector(glm::vec<N,T>{ v });
    }
    template <Quantity Q>
        requires std::constructible_from<T, typename quantity_vector<N,Q>::R>
    explicit constexpr inline unit_vector(const quantity_vector<N, Q>& qv) {
        this->x = T{ qv[0].numerical_value_in(u::one) };
        if constexpr (N>=2) this->y = T{ qv[1].numerical_value_in(u::one) };
        if constexpr (N>=3) this->z = T{ qv[2].numerical_value_in(u::one) };
        
        check_unit_vector(glm::vec<N,T>{ qv });
    }

    constexpr unit_vector(unit_vector&&) noexcept = default;
    constexpr unit_vector(const unit_vector&) noexcept = default;
    constexpr unit_vector& operator=(unit_vector&&) noexcept = default;
    constexpr unit_vector& operator=(const unit_vector&) noexcept = default;

    constexpr inline const auto& operator[](std::size_t i) const noexcept {
        assert(i<N);
        if constexpr (N>=1) if (i==0) return this->x;
        if constexpr (N>=2) if (i==1) return this->y;
        if constexpr (N>=3) if (i==2) return this->z;
        
        unreachable();
    }

    template <std::size_t M, typename S>
        requires (M<=N) && std::convertible_to<S, T>
    explicit constexpr inline operator vec<M,S>() const noexcept {
        vec<M,S> ret;
        for (auto i=0ul;i<M;++i)
            ret[i] = static_cast<S>((*this)[i]);
        return ret;
    }
    template <FloatingPoint S>
        requires std::convertible_to<S, T>
    explicit constexpr inline operator unit_vector<N, S>() const noexcept {
        vec<N, S> ret{};
        for (auto i=0ul;i<N;++i)
            ret[i] = static_cast<S>((*this)[i]);
        return unit_vector<N, S>{ ret };
    }
};


namespace detail {

template <std::size_t N, typename T>
void to_base_specialization_of_unit_vector(const unit_vector<N, T>*);

template<typename T>
constexpr bool is_derived_from_specialization_of_unit_vector = 
    requires(T* type) { to_base_specialization_of_unit_vector(type); };

}

template <typename T>
concept UnitVector = detail::is_derived_from_specialization_of_unit_vector<T>;
template<typename T>
concept VectorOrUnitVector = NumericVector<T> || UnitVector<T>;
template<typename T>
concept Vector = NumericVector<T> || UnitVector<T> || QuantityVector<T>;


template <std::size_t N, FloatingPoint T>
using dirvec = unit_vector<N,T>;

template <FloatingPoint T>
using dirvec1 = unit_vector<1,T>;
template <FloatingPoint T>
using dirvec2 = unit_vector<2,T>;
template <FloatingPoint T>
using dirvec3 = unit_vector<3,T>;

/**
 * @brief 1-dimensional unitless direction vector (can be assumed to be always normalized)
 */
using dir1_t = dirvec1<f_t>;
/**
 * @brief 2-dimensional unitless direction vector (can be assumed to be always normalized)
 */
using dir2_t = dirvec2<f_t>;
/**
 * @brief 3-dimensional unitless direction vector (can be assumed to be always normalized)
 */
using dir3_t = dirvec3<f_t>;

/**
 * @brief 1-dimensional unitless direction vector (can be assumed to be always normalized)
 */
using dir1d_t = dirvec1<double>;
/**
 * @brief 2-dimensional unitless direction vector (can be assumed to be always normalized)
 */
using dir2d_t = dirvec2<double>;
/**
 * @brief 3-dimensional unitless direction vector (can be assumed to be always normalized)
 */
using dir3d_t = dirvec3<double>;

}
