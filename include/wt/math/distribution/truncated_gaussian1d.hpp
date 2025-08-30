/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cmath>
#include <gcem.hpp>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>
#include <wt/math/erf_lut.hpp>
#include "distribution1d.hpp"

namespace wt {

/**
 * @brief 1D truncated Gaussian distribution.
 *        Correctly handles the singular case where the standard deviation is 0 and the distribution becomes a Dirac.
 *        Supports single-sided truncation.
 */
class truncated_gaussian1d_t final : public distribution1d_t {
private:
    f_t mu;
    f_t sigma;
    f_t recp_sigma;

    f_t alpha,beta;
    f_t PsiA, PsiB, recp_Z;

    static inline auto phi(f_t x) noexcept { return m::inv_sqrt_two_pi * m::exp(-f_t(.5)*x*x); }
    static inline f_t Psi(f_t x) noexcept {
        if (x==-m::inf) return 0;
        if (x==0)       return f_t(.5);
        if (x==+m::inf) return 1;
        return f_t(.5) * (1 + m::erf_lut(x * m::inv_sqrt_two));
    }
    static inline auto inversePsi(f_t x) noexcept { return m::sqrt_two * gcem::erf_inv(2*x-1); }

    [[nodiscard]] inline auto Z() const noexcept { return PsiB-PsiA; }

public:
    /**
     * @brief Construct a new truncated Gaussian distribution.
     * 
     * @param sigma standard deviation
     * @param mu mean
     * @param range distribution range, may be of infinite length.
     */
    truncated_gaussian1d_t(f_t sigma, 
                           f_t mu,
                           const range_t<>& range)
        : mu(mu),
          sigma(sigma),
          recp_sigma(f_t(1)/sigma),
          alpha((range.min-mu) * (sigma>0?recp_sigma:1)),
          beta((range.max-mu) * (sigma>0?recp_sigma:1)),
          PsiA(Psi(alpha)),
          PsiB(Psi(beta)),
          recp_Z(f_t(1)/(PsiB-PsiA))
    {}

    truncated_gaussian1d_t(const truncated_gaussian1d_t&) noexcept = default;
    truncated_gaussian1d_t &operator=(const truncated_gaussian1d_t&) noexcept = default;

    [[nodiscard]] std::unique_ptr<distribution1d_t> clone() const override {
        return std::make_unique<truncated_gaussian1d_t>(*this);
    }

    /**
     * @brief The mean of the Gaussian distribution.
     */
    [[nodiscard]] inline auto mean() const noexcept {
        return is_dirac() ? mu : 
               mu + sigma*recp_Z*(phi(alpha)-phi(beta));
    }
    /**
     * @brief The standard deviation of the truncated Gaussian distribution.
     */
    [[nodiscard]] inline f_t std_dev() const noexcept {
        if (is_dirac()) return 0;

        const auto pa = phi(alpha);
        const auto pb = phi(beta);
        const auto r  = pa-pb;
        const auto apa = m::isfinite(alpha) ? alpha*pa : 0;
        const auto bpb = m::isfinite(beta)  ? beta*pb  : 0;
        return sigma * m::sqrt(m::max<f_t>(0,
                            1 - 
                            recp_Z * (bpb-apa) - 
                            m::sqr(recp_Z * r)
                       ));
    }

    /**
     * @brief PDF
     * @param x value
     * @return PDF of the distribution at x
     */
    [[nodiscard]] inline f_t pdf(f_t x, 
                                 measure_e measure = measure_e::continuos) const noexcept final {
        const auto xi = (x-mu) * recp_sigma;
        return 
            !is_dirac() ? 
            (measure==measure_e::continuos ? 
                phi(xi)*recp_sigma*recp_Z :
                0
             ) :
            (x==mu && alpha<=0&&beta>=0 && measure==measure_e::discrete ? +m::inf : 0);
    }

    /**
     * @brief Samples a Gaussian distributed point.
     */
    [[nodiscard]] inline sample_ret_t sample(sampler::sampler_t& sampler) const noexcept final {
        if (is_dirac()) {
            return sample_ret_t{
                .x = mu,
                .measure = measure_e::discrete,
                .pdf = 1.
            };
        }

        const auto r = sampler.r();
        const auto pt = sigma * inversePsi(PsiA + r*(PsiB-PsiA)) + mu;
        return sample_ret_t{
            .x = pt,
            .measure = measure_e::continuos,
            .pdf = pdf(pt),
        };
    }

    /**
     * @brief Integrates this Gaussian distribution over a range.
     */
    [[nodiscard]] inline f_t integrate(const range_t<>& r) const noexcept {
        if (is_dirac()) {
            return r.contains(mu) && alpha<=0&&beta>=0 ? 1 : 0;
        }

        const auto xi1 = m::clamp((r.min-mu) * recp_sigma, alpha,beta);
        const auto xi2 = m::clamp((r.max-mu) * recp_sigma, alpha,beta);

        return (Psi(xi2)-Psi(xi1)) * recp_Z;
    }

    /**
     * @brief Returns true if the distribution is degenerate (a Dirac delta)
     */
    [[nodiscard]] inline bool is_dirac() const noexcept { return sigma==0; }
};

}
