/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/sampler/sampler.hpp>
#include <wt/sampler/measure.hpp>

#include <wt/math/common.hpp>

#include <wt/util/assert.hpp>

namespace wt {

/**
 * @brief 2D Gaussian distribution.
 *        Correctly handles the singular case where the standard deviation is 0 and the distribution becomes a Dirac.
 */
class gaussian2d_t {
private:
    vec2_t mu;
    dir2_t x;
    vec2_t sigma;
    vec2_t recp_sigma;
    f_t gaussian_norm;

public:
    /**
     * @brief Construct a new 2D Gaussian distribution.
     * 
     * @param sigma standard deviation
     * @param x direction of the x-component of `sigma`
     * @param mu mean
     */
    constexpr gaussian2d_t(vec2_t sigma = { 1,1 }, 
                           dir2_t x     = { 1,0 },
                           vec2_t mu    = { 0,0 })
        : mu(mu),
          x(x),
          sigma(sigma),
          recp_sigma(f_t(1)/sigma),
          gaussian_norm(m::inv_two_pi * recp_sigma.x*recp_sigma.y)
    {}

    gaussian2d_t(const gaussian2d_t&) noexcept = default;
    gaussian2d_t &operator=(const gaussian2d_t&) noexcept = default;

    /**
     * @brief Creates a distribution with its mean and stddev scaled by a constant
     */
    [[nodiscard]] constexpr gaussian2d_t scaled(f_t s) const noexcept {
        return gaussian2d_t{ sigma*s, x, mu*s };
    }

    /**
     * @brief Creates a distribution with its mean and stddev scaled by constants
     */
    [[nodiscard]] constexpr gaussian2d_t scaled(f_t mean_scale, f_t sigma_scale) const noexcept {
        return gaussian2d_t{ sigma*sigma_scale, x, mu*mean_scale };
    }

    /**
     * @brief The mean of the Gaussian distribution.
     */
    [[nodiscard]] inline constexpr auto mean() const noexcept { return mu; }
    /**
     * @brief The standard deviations of the Gaussian distribution.
     */
    [[nodiscard]] inline constexpr auto std_dev() const noexcept { return sigma; }
    /**
     * @brief The reciprocal standard deviations of the Gaussian distribution.
     */
    [[nodiscard]] inline constexpr auto recp_std_dev() const noexcept { return recp_sigma; }
    /**
     * @brief The local frame (first/second std_dev aligns with the x/y-axis) in form of a 2D rotation matrix.
     */
    [[nodiscard]] inline constexpr mat2_t frame_mat() const noexcept {
        return mat2_t(vec2_t{ x },vec2_t{ -x.y,x.x });
    }
    /**
     * @brief The inverse of the Gaussian covariance matrix (Sigma).
     */
    [[nodiscard]] inline auto invSigma() const noexcept {
        const auto R = frame_mat();
        return R * 
               mat2_t(m::sqr(recp_sigma.x),0,0,m::sqr(recp_sigma.y)) * 
               m::transpose(R);
    }

    [[nodiscard]] inline constexpr auto pdf(const vec2_t& p) const noexcept {
        const auto u = p * recp_sigma;
        return 
            !is_dirac() ? gaussian_norm * m::exp(-m::dot(u,u)/2) :
            (p==mu ? m::inf : 0);
    }

    struct sample_ret_t {
        vec2_t pt;
        measure_e measure;
        f_t pdf;
    };
    /**
     * @brief Samples a Gaussian distributed point.
     */
    [[nodiscard]] inline constexpr sample_ret_t sample(sampler::sampler_t& sampler) const noexcept {
        if (is_dirac()) {
            return sample_ret_t{
                .pt = mu,
                .measure = measure_e::discrete,
                .pdf = 1.
            };
        }

        const auto pt = sampler.normal2d();
        return sample_ret_t{
            .pt = from_canonical(pt),
            .measure = measure_e::continuos,
            .pdf = sampler.normal2d_pdf(pt),
        };
    }

    /**
     * @brief Computes the p-norm of the Gaussian.
     */
    template <f_t p>
    [[nodiscard]] inline constexpr auto p_norm() const noexcept {
        if constexpr (p==1) return 1;
        return gaussian_norm / m::pow(p*gaussian_norm, 1/p);
    }

    /**
     * @brief Integrates this Gaussian distribution over the support of another Gaussian distribution.
     *
     * @param equal_means Assume that both distributions have equal means, and optimize accordingly.
     * @param no_diracs Assume that both distributions are not Diracs. Allows to skip a few tests.
     */
    template <bool equal_means=false, bool no_diracs=false>
    [[nodiscard]] inline f_t integrate(const gaussian2d_t& g) const noexcept {
        const auto mu = equal_means ? vec2_t{ 0,0 } : g.mean() - mean();

        // handle degenerate distributions
        if constexpr (!no_diracs) {
            if (is_dirac() && g.is_dirac()) 
                return m::all(m::iszero(mu)) ? m::inf : 0;
            if (is_dirac()) return g.pdf(mean());
            if (g.is_dirac()) return pdf(g.mean());
        }

        const auto& S1 = invSigma();
        const auto& S2 = g.invSigma();
        const auto S1S2 = S1+S2;
        const auto recp_det = 1/m::determinant(S1S2);

        const auto denom = m::two_pi * gaussian_norm * g.gaussian_norm * m::sqrt(recp_det);
        if constexpr (equal_means)
            return denom;

        const auto invS1S2 = recp_det * mat2_t(S1S2[1][1],-S1S2[0][1],-S1S2[0][1],S1S2[0][0]);
        const auto invS = S1 * invS1S2 * S2;
        return denom * m::exp(-m::dot(mu,invS*mu)/2);
    }

    /**
     * @brief Integrates the Gaussian over the support of a triangle defined via its 3 2D vertices.
     *        Expensive. Works with arbitrary triangles, accuracy usually within 1-3% rel. err.
     */
    [[nodiscard]] f_t integrate_triangle(vec2_t a, vec2_t b, vec2_t c) const noexcept;

    /**
     * @brief Returns true if the distribution is degenerate (a Dirac delta)
     */
    [[nodiscard]] inline constexpr bool is_dirac() const noexcept { return sigma.x==0 || sigma.y==0; }

    [[nodiscard]] inline constexpr vec2_t to_local(const vec2_t& v) const noexcept {
        return vec2_t{ 
            m::dot(x,v), 
            m::dot(vec2_t{ -x.y,x.x },v) 
        };
    }
    [[nodiscard]] inline constexpr vec2_t from_local(const vec2_t& v) const noexcept {
        return x*v.x + vec2_t{ -x.y,x.x }*v.y;
    }

    [[nodiscard]] inline constexpr vec2_t to_canonical(const vec2_t& v) const noexcept {
        const auto p = vec2_t{ 
            m::dot(x,v-mu), 
            m::dot(vec2_t{ -x.y,x.x },v-mu) 
        };
        if (!is_dirac())
            return p * recp_sigma;
        return { p.x==0 ? 0 : m::inf, p.y==0 ? 0 : m::inf };
    }
    [[nodiscard]] inline constexpr vec2_t from_canonical(const vec2_t& v) const noexcept {
        return sigma * (x*v.x + vec2_t{ -x.y,x.x }*v.y) + mu;
    }
};

}
