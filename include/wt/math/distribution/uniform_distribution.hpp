/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cassert>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>
#include "distribution1d.hpp"

namespace wt {

/**
 * @brief Continuos uniform distribution.
 */
class uniform_distribution_t final : public distribution1d_t {
public:
    using range_t = wt::range_t<f_t, range_inclusiveness_e::inclusive>;

private:
    range_t dist_range;
    f_t recp_range_length;

public:
    /**
     * @brief Construct a uniform_distribution_t
     */
    explicit uniform_distribution_t(const range_t range) 
        : dist_range(range),
          recp_range_length(f_t(1) / dist_range.length())
    {
        assert(!range.empty());
    }

    uniform_distribution_t(uniform_distribution_t&&) noexcept = default;
    uniform_distribution_t(const uniform_distribution_t&) noexcept = default;
    uniform_distribution_t& operator=(uniform_distribution_t&&) noexcept = default;
    uniform_distribution_t& operator=(const uniform_distribution_t&) noexcept = default;

    [[nodiscard]] std::unique_ptr<distribution1d_t> clone() const override {
        return std::make_unique<uniform_distribution_t>(*this);
    }

    [[nodiscard]] const auto& range() const noexcept { return dist_range; }

    /**
     * @brief PDF
     * @param x value
     * @return PDF of the distribution at x
     */
    [[nodiscard]] inline f_t pdf(f_t x, 
                                 measure_e measure = measure_e::continuos) const noexcept final {
        return measure==measure_e::continuos && dist_range.contains(x) ? 
            recp_range_length :
            0;
    }

    /**
     * @brief CDF
     */
    [[nodiscard]] inline auto cdf(f_t val) const noexcept {
        return val>=dist_range.max ? 1 :
               val<=dist_range.min ? 0 :
               (val-dist_range.min) * recp_range_length;
    }

    /**
     * @brief Inverse CDF
     */
    [[nodiscard]] inline f_t icdf(f_t v) const noexcept {
        assert(dist_range.length()<m::inf && dist_range.length()>=0);
        return m::mix(dist_range,v);
    }
    /**
     * @brief Sample from the distribution
     * @return sampled x,y value
     */
    [[nodiscard]] sample_ret_t sample(sampler::sampler_t& sampler) const noexcept final {
        return {
            .x = icdf(sampler.r()),
            .measure = measure_e::continuos,
            .pdf = recp_range_length,
        };
    }
};

}
