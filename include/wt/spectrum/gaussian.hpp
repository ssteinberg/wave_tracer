/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <string>
#include <memory>

#include <wt/sampler/sampler.hpp>

#include <wt/math/common.hpp>
#include <wt/math/distribution/truncated_gaussian1d.hpp>
#include <wt/wt_context.hpp>

#include "spectrum.hpp"

namespace wt::spectrum {

/**
 * @brief Gaussian (real-valued) spectrum. Uses an analytic truncated Gaussian distribution underneath, providing correct power() queries and sampling facilities.
 */
class gaussian_t final : public spectrum_real_t {
private:
    truncated_gaussian1d_t dist;
    f_t val0, recp_g0;
    const range_t<wavenumber_t> range;

public:
    gaussian_t(std::string id, 
               const truncated_gaussian1d_t& dist,
               const f_t val0,
               const range_t<wavenumber_t>& range)
        : spectrum_real_t(std::move(id)),
          dist(dist),
          val0(val0),
          recp_g0(val0 / dist.pdf(dist.mean())),
          range(range)
    {
        assert(!dist.is_dirac());
    }
    gaussian_t(const gaussian_t&) = default;
    gaussian_t(gaussian_t&&) = default;

    /**
     * @brief Returns the underlying spectrum distribution.
     */
    [[nodiscard]] const distribution1d_t* distribution() const noexcept override {
        return &dist;
    }

    /**
     * @brief Returns the total spectral power contained in this spectrum.
     */
    [[nodiscard]] f_t power() const noexcept override {
        return power(range);
    }

    /**
     * @brief Returns the spectral power over the provided wavenumber range.
     */
    [[nodiscard]] f_t power(const range_t<wavenumber_t>& wavenumbers) const noexcept override {
        const auto r = range_t{ 
            m::max<f_t>(0,u::to_inv_mm(wavenumbers.min)), 
            u::to_inv_mm(wavenumbers.max)
        };

        if (dist.is_dirac())
            return r.contains(dist.mean()) ? val0 : 0;
        return recp_g0 * dist.integrate(r);
    }

    /**
     * @brief Returns the range of wavenumbers for which this spectrum is defined.
     *        Querying the spectrum with wavenumber<range.min or wavenumber>range.max will always be 0.
     */
    [[nodiscard]] range_t<wavenumber_t> wavenumber_range() const noexcept override {
        return range;
    }
    
    /**
     * @brief Returns the mean wavenumber for this spectrum.
     */
    [[nodiscard]] wavenumber_t mean_wavenumber() const noexcept override {
        return dist.mean() / u::mm;
    }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] f_t f(const wavenumber_t wavenumber) const noexcept override {
        const auto x = u::to_inv_mm(wavenumber);

        if (dist.is_dirac())
            return x == dist.mean() ? val0 : 0;
        return x>0 ?
            recp_g0 * dist.pdf(x) : 
            0;
    }
    
public:
    static std::unique_ptr<spectrum_real_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
