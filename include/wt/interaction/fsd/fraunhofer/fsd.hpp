/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <vector>
#include <wt/math/common.hpp>

namespace wt::fraunhofer::fsd {

/**
 * @brief Edge parametrising a free-space diffraction (FSD) angular scattering function.
 */
struct edge_t {
    vec2_t e;        // edge vector
    vec2_t v;        // mid point

    c_t a_b, iab_2;  // beam amplitudes

    // tangent vector (premultiplied by wavenumber)
    [[nodiscard]] inline constexpr auto m() const noexcept {
        return vec2_t{ e.y,-e.x };
    }
    // Ξ matrix (premultiplied by wavenumber)
    [[nodiscard]] inline constexpr auto Xi() const noexcept {
        return mat2_t(e, m());
    }
};

/**
 * @brief An FSD aperture
 */
struct fsd_aperture_t {
    std::vector<edge_t> edges;  // aperture edges

    std::vector<f_t> edge_pdfs; // edge selection pdfs for sampling
    f_t P0;
    f_t P0_pdf;                 // PDF of selecting 0-th order lobe

    f_t psi02;                  // complex magnitude squared of integrated field amplitude (over the aperture opening)
    f_t recp_I;                 // reciprocal of total incident beam intensity (over the aperture opening)

    inline void reserve(std::size_t n) noexcept {
        edges.reserve(n);
        edge_pdfs.reserve(n);
    }

    [[nodiscard]] inline bool single_edge() const noexcept { return edges.size()==1; }
};


// Power contained in χₑ×|alpha1|²
static constexpr auto PA1 = f_t(0.0049361075794549872500);
// Power contained in χₑ×|alpha2|²
static constexpr auto PA2 = f_t(0.21899789398059305541);

static constexpr auto P0_sigma = f_t(0.288675134594813)/4; // 1. / sqrtf(12.)/4;

inline constexpr f_t alpha1(f_t x,f_t y) noexcept {
    return x==0 ? 
        0 : 
        m::inv_two_pi * y/(x*(x*x+y*y)) * (m::cos(x/2*u::ang::rad)-m::sinc(x/2));
}
inline constexpr f_t alpha1(vec2_t zeta) noexcept { return alpha1(zeta.x,zeta.y); }
inline constexpr f_t alpha2(f_t x,f_t y) noexcept {
    return x==0 ? 
        0 : 
        m::inv_two_pi * y/(x*x+y*y) * m::sinc(x/2);
}
inline constexpr f_t alpha2(vec2_t zeta) noexcept { return alpha2(zeta.x,zeta.y); }

/**
 * @brief The masking function for the diffracted lobes.
 */
inline constexpr f_t chi_e(vec2_t xi) noexcept {
    constexpr f_t chi = 0.830092714835359;
    const auto xi2 = m::dot(xi,xi);
    const auto t  = 1+chi*xi2;
    const auto t2 = t*t;
    const auto t3 = t2*t;

    return m::max<f_t>(0,1 - (3/t2 - 2/t3));
}
/**
 * @brief The masking function for the 0-th order lobe lobes.
 */
inline constexpr f_t chi_0(vec2_t xi) noexcept {
    xi /= P0_sigma;
    const auto xi2 = m::dot(xi,xi);
    return m::exp(-f_t(.5) * xi2);
}

/**
 * @brief Evaluates the Ѱ function of the FSD diffraction function.
 *        (does not include the 0-th order lobe)
 */
[[nodiscard]] inline c_t Psi(const edge_t& e, const vec2_t xi) noexcept {
    const auto zeta = xi * e.Xi();

    const auto a1 = e.a_b   * alpha1(zeta);
    const auto a2 = e.iab_2 * alpha2(zeta);

    const auto ee2 = m::length2(e.e);
    const auto vxi = m::dot(e.v,xi);

    return std::polar<f_t>(ee2, -vxi) * (a1 + a2);
}

/**
 * @brief Approximates the |Ѱ|² scattering function. 
 *        (does not include the 0-th order lobe)
 */
[[nodiscard]] inline f_t Psi2(const fsd::edge_t& e, const vec2_t xi) noexcept {
    const auto zeta = xi * e.Xi();

    const auto a1 = e.a_b   * alpha1(zeta);
    const auto a2 = e.iab_2 * alpha2(zeta);

    const f_t ee2 = m::length2(e.e);

    return m::sqr(ee2) * std::norm(a1 + a2);
}
/**
 * @brief Approximates the |Ѱ|² scattering function. 
 *        (includes the 0-th order lobe)
 */
[[nodiscard]] inline f_t sampling_density(const fsd_aperture_t& aperture, const vec2_t xi) noexcept {
    f_t diffracted = 0;
    for (const auto& e : aperture.edges) diffracted += Psi2(e, xi);

    return diffracted * chi_e(xi) + aperture.P0 * m::inv_two_pi / m::sqr(P0_sigma) * chi_0(xi);
}

/**
 * @brief Evaluates the free-space diffraction ASF (angular scattering function)
 *        Only uses edge diffractions: unstable around xi=0
 */
[[nodiscard]] inline f_t ASF_unclamped(const fsd_aperture_t& aperture, vec2_t xi) noexcept {
    c_t amplitude = 0;
    for (const auto& e : aperture.edges) amplitude += Psi(e, xi);

    return std::norm(amplitude);
}

/**
 * @brief Evaluates the free-space diffraction ASF (angular scattering function)
 */
[[nodiscard]] inline f_t ASF(const fsd_aperture_t& aperture, vec2_t xi) noexcept {
    const auto diffracted = ASF_unclamped(aperture, xi);
    return diffracted * chi_e(xi) + aperture.psi02 * chi_0(xi);
}

/**
 * @brief Power in 0-th order lobe
 */
inline constexpr f_t P0(const fsd_aperture_t& aperture) { 
    return m::two_pi * m::sqr(P0_sigma) * aperture.psi02;
}
/**
 * @brief Power in edge's χₑ×|𝛂1|² lobe  (with 0-th order lobe removed)
 */
inline constexpr f_t Pa1(const edge_t& edge) { 
    return m::sqr(m::length2(edge.e)) * PA1 * std::norm(edge.a_b);
}
/**
 * @brief Power in edge's χₑ×|𝛂2|² lobe  (with 0-th order lobe removed)
 */
inline constexpr f_t Pa2(const edge_t& edge) { 
    return m::sqr(m::length2(edge.e)) * PA2 * std::norm(edge.iab_2);
}
/**
 * @brief Approximates the scattered power contained in an edge: χₑ×(|𝛂1|² + |𝛂2|²).
 *        (that is, in an aperture that contains a single edge. Scattered power of multi-edge aperture is not linear, due to interference.)
 *        Ignores the (2 Re 𝛂1×𝛂2) cross-term term, which is negligible (~1e-8 relative error).
 *        (does not include the 0-th order lobe)
 */
inline constexpr f_t Pj(const edge_t& edge) { 
    return Pa1(edge) + Pa2(edge);
}

}
