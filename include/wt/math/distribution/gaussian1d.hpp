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

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>
#include <wt/math/erf_lut.hpp>
#include "distribution1d.hpp"

namespace wt {

/**
 * @brief 1D Gaussian distribution.
 *        Correctly handles the singular case where the standard deviation is 0 and the distribution becomes a Dirac.
 */
class gaussian1d_t final : public distribution1d_t {
private:
    f_t mu;
    f_t sigma;
    f_t recp_sigma;

public:
    /**
     * @brief Construct a new 1D Gaussian distribution.
     * 
     * @param sigma standard deviation
     * @param mu mean
     */
    gaussian1d_t(f_t sigma, 
                 f_t mu = 0)
        : mu(mu),
          sigma(sigma),
          recp_sigma(f_t(1)/sigma)
    {}

    gaussian1d_t(const gaussian1d_t&) noexcept = default;
    gaussian1d_t &operator=(const gaussian1d_t&) noexcept = default;

    [[nodiscard]] std::unique_ptr<distribution1d_t> clone() const override {
        return std::make_unique<gaussian1d_t>(*this);
    }

    /**
     * @brief The mean of the Gaussian distribution.
     */
    [[nodiscard]] inline auto mean() const noexcept { return mu; }
    /**
     * @brief The standard deviations of the Gaussian distribution.
     */
    [[nodiscard]] inline auto std_dev() const noexcept { return sigma; }

    /**
     * @brief PDF
     * @param x value
     * @return PDF of the distribution at x
     */
    [[nodiscard]] inline f_t pdf(f_t x, 
                                 measure_e measure = measure_e::continuos) const noexcept final {
        return 
            !is_dirac() ? 
            (measure==measure_e::continuos ? 
                m::inv_sqrt_two_pi*recp_sigma * m::exp(-m::sqr((x-mu)*recp_sigma)/2) :
                0
             ) :
            (x==mu && measure==measure_e::discrete ? +m::inf : 0);
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

        const auto pt = sigma*sampler.normal2d().x + mu;
        return sample_ret_t{
            .x = pt,
            .measure = measure_e::continuos,
            .pdf = pdf(pt),
        };
    }

    /**
     * @brief Integrates this Gaussian distribution over a range.
     */
    [[nodiscard]] inline f_t integrate(const range_t<f_t>& r) const noexcept {
        if (is_dirac())
            return r.contains(mu) ? 1 : 0;
        
        const auto n = m::inv_sqrt_two * recp_sigma;
        return (m::erf_lut((r.max-mu)*n) - m::erf_lut((r.min-mu)*n)) / 2;
    }

    /**
     * @brief Integrates this Gaussian distribution over the support of another Gaussian distribution.
     */
    [[nodiscard]] inline f_t integrate(const gaussian1d_t& g) const noexcept {
        assert(!is_dirac() && !g.is_dirac());

        const auto recp_sigma2 = 1/(m::sqr(sigma)+m::sqr(g.sigma));
        return m::inv_sqrt_two_pi * m::sqrt(recp_sigma2) * 
               (mu==g.mu ? 1 : m::exp(-m::sqr(mu-g.mu)*recp_sigma2/2));
    }

    /**
     * @brief Returns true if the distribution is degenerate (a Dirac delta)
     */
    [[nodiscard]] inline bool is_dirac() const noexcept { return sigma==0; }
};

}
