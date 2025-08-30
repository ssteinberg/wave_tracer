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
#include <wt/math/distribution/piecewise_linear_distribution.hpp>
#include <wt/wt_context.hpp>

#include "spectrum.hpp"

namespace wt::spectrum {

/**
 * @brief Piece-wise linear (real-valued) spectrum.
 */
class piecewise_linear_t final : public spectrum_real_t {
private:
    piecewise_linear_distribution_t dist;

public:
    piecewise_linear_t(std::string id,
                       piecewise_linear_distribution_t&& dist)
        : spectrum_real_t(std::move(id)),
          dist(std::move(dist))
    {}
    piecewise_linear_t(const piecewise_linear_t&) = default;
    piecewise_linear_t(piecewise_linear_t&&) = default;

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
        return dist.total();
    }

    /**
     * @brief Returns the average spectral power over the provided wavenumber range.
     */
    [[nodiscard]] f_t power(const range_t<wavenumber_t>& wavenumbers) const noexcept override {
        const auto p = dist.integrate(
            u::to_inv_mm(wavenumbers.min), 
            u::to_inv_mm(wavenumbers.max)
        );
        return p;
    }

    /**
     * @brief Returns the range of wavenumbers for which this spectrum is defined.
     *        Querying the spectrum with wavenumber<range.min or wavenumber>range.max will always be 0.
     */
    [[nodiscard]] range_t<wavenumber_t> wavenumber_range() const noexcept override {
        const auto rng = dist.range();
        return {
            rng.min / u::mm,
            rng.max / u::mm
        };
    }
    
    /**
     * @brief Returns the mean wavenumber for this spectrum.
     */
    [[nodiscard]] wavenumber_t mean_wavenumber() const noexcept override {
        return dist.icdf(.5).x / u::mm;
    }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] f_t f(const wavenumber_t wavenumber) const noexcept override {
        return wavenumber>zero ? dist.value(u::to_inv_mm(wavenumber)) : 0;
    }
    
public:
    static std::unique_ptr<spectrum_real_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
