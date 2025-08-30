/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <optional>

#include <wt/math/common.hpp>
#include <wt/math/eft/eft.hpp>

namespace wt {

struct QR_ret_t {
    f_t Qcos, Qsin;
    mat2_t R;
};

inline auto QR(const mat2_t& A) noexcept {
    auto a = A[0][0];
    auto b = A[1][0];
    auto c = A[0][1];
    auto d = A[1][1];

    f_t x,y,z,Qc,Qs;
    if (c==0) {
        x = a;
        y = b;
        z = d;
        Qc = 1;
        Qs = 0;
    }
    else {
        const auto m = m::max(m::abs(c), m::abs(d));
        const auto recp_m = 1/m;
        c *= recp_m;
        d *= recp_m;

        const auto r = m::sqrt(c*c+d*d);
        const auto l = f_t(1)/r;
        x = m::eft::diff_prod(a,d, b,c)*l;
        y = m::eft::sum_prod(a,c, b,d)*l;
        z = m*r;
        Qs = -c*l;
        Qc =  d*l;
    }

    return QR_ret_t{
        .Qcos = Qc,
        .Qsin = Qs,
        .R    = mat2_t{x,0,y,z}, 
    };
}

/**
 * @brief SVD decomposition for a matrix \f$ A = U \Sigma V \f$.
 *        The components for the orthogonal matrices are given as
 *          \f$ U = \begin{matrix} U_\text{cos} & U_\text{sin} \\ -U_\text{sin} & U_\text{cos} \end{matrix} \f$,
 *        and similarly for \f$ V \f$.
 * 
 */
struct SVD_ret_t {
    f_t Ucos, Usin;
    f_t Vcos, Vsin;
    f_t sigma1, sigma2;
};

/**
 * @brief Singular value decomposition of a 2X2 matrix. Branchless (essentially) and trigonometric functions free.
 *        From: https://scicomp.stackexchange.com/questions/8899/robust-algorithm-for-2-times-2-svd
 */
inline auto SVD(const mat2_t& A) {
    const auto qr = QR(A);
    const auto x = qr.R[0][0];
    const auto y = qr.R[1][0];
    const auto z = qr.R[1][1];
    auto c2 = qr.Qcos;
    auto s2 = qr.Qsin;

    const auto n = m::max(m::abs(x), m::abs(y));
    if (n==0) {
        return SVD_ret_t{
            .Ucos = 1,
            .Usin = 0,
            .Vcos = c2,
            .Vsin = s2,
            .sigma1 = A[0][0],
            .sigma2 = A[1][1],
        };
    }

    const auto numer = (z-x)*(z+x) + m::sqr(y);
    const auto tt  = numer!=0 ? numer/(n*x*y) : 0;
    auto t = 2*f_t(tt>=0?1:-1)/(m::abs(tt) + m::sqrt(m::sqr(tt)+4));
    auto c1 = 1/m::sqrt(1 + m::sqr(t));
    auto s1 = c1*t;

    const auto usa = m::eft::diff_prod(c1,x, s1,y); 
    const auto usb = m::eft::sum_prod(s1,x, c1,y);
    const auto usc = -s1*z;
    const auto usd = c1*z;

    t = m::eft::sum_prod(c1,c2, s1,s2);
    s2 = m::eft::diff_prod(c2,s1, c1,s2);
    c2 = t;

    auto sigma1 = m::sqrt(m::sqr(usa) + m::sqr(usc));
    auto sigma2 = m::sqrt(m::sqr(usb) + m::sqr(usd));
    auto dmax = m::max(sigma1, sigma2);
    const auto usmax1 = sigma2>sigma1 ? usd :  usa;
    const auto usmax2 = sigma2>sigma1 ? usb : -usc;

    const auto signsigma1 = f_t(x*z>0 ? 1 : -1);
    dmax *= sigma2 > sigma1 ? signsigma1 : 1;
    sigma2 *= signsigma1;
    const auto r = 1/dmax;

    return SVD_ret_t{
        .Ucos = dmax!=0 ? usmax1*r : 1,
        .Usin = dmax!=0 ? usmax2*r : 0,
        .Vcos = c2,
        .Vsin = s2,
        .sigma1 = sigma1,
        .sigma2 = sigma2,
    };
}

/**
 * @brief Eigen values decomposition of a 2X2 matrix. 
 */
inline auto eigen_values(const mat2_t& A) {
    const auto d = m::determinant(A);
    const auto t = A[0][0] + A[1][1];

    const auto q = m::sqrt(m::sqr(t/2)-d);
    return vec2_t{
        t/2 + q,
        t/2 - q
    };
}

/**
 * @brief Solves the linear system Ax=b for given 2x2 matrix A and 2-element vector b.
 *        If a solution exists returns x, otherwise returns std::nullopt.
 * 
 */
template <FloatingPoint T>
inline std::optional<vec<2,T>> solve_linear_system2x2(const mat2<T>& A, const vec2<T>& b) noexcept {
    const auto det = m::determinant(A);
    if (m::abs(det)<limits<T>::epsilon()) 
        return std::nullopt;

    const auto recp_det = T(1)/det;
    const auto x = recp_det * vec2_t{
        m::eft::diff_prod(A[1][1],b.x, A[1][0],b.y),
        m::eft::diff_prod(A[0][0],b.y, A[0][1],b.x),
    };
    return x;
}

}
