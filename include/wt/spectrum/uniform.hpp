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
#include <wt/math/distribution/uniform_distribution.hpp>
#include <wt/wt_context.hpp>

#include "spectrum.hpp"

namespace wt::spectrum {

/**
 * @brief Uniform (real-valued) spectrum: returns a fixed real constant for all wavenumbers within the range.
 *        Range may include all non-negative wavenumbers, \f$ k \in [0\ \textrm{mm}^{-1},+\infty\ \textrm{mm}^{-1}) \f$, or any subset.
 */
class uniform_t final : public spectrum_real_t {
private:
    uniform_distribution_t dist;
    f_t avg_spectral_pwr;

public:
    uniform_t(std::string id, f_t avg_pwr,
              const range_t<wavenumber_t> krange = range_t<wavenumber_t>::positive())
        : spectrum_real_t(std::move(id)),
          dist(uniform_distribution_t{ { u::to_inv_mm(krange.min), u::to_inv_mm(krange.max) } }),
          avg_spectral_pwr(avg_pwr)
    {}
    uniform_t(const uniform_t&) = default;
    uniform_t(uniform_t&&) = default;

    /**
     * @brief Returns the underlying spectrum distribution.
     */
    [[nodiscard]] const distribution1d_t* distribution() const noexcept override {
        return &dist;
    }

    /**
     * @brief Returns the average spectral power contained in this spectrum.
     */
    [[nodiscard]] f_t average_power() const noexcept { return avg_spectral_pwr; }

    /**
     * @brief Returns the total spectral power contained in this spectrum.
     */
    [[nodiscard]] f_t power() const noexcept override {
        return dist.range().length() * avg_spectral_pwr;
    }

    /**
     * @brief Returns the spectral power over the provided wavenumber range.
     */
    [[nodiscard]] f_t power(const range_t<wavenumber_t>& wavenumbers) const noexcept override {
        return avg_spectral_pwr * u::to_inv_mm((wavenumber_range() & wavenumbers).length());
    }

    /**
     * @brief Returns the range of wavenumbers for which this spectrum is defined.
     *        Querying the spectrum with wavenumber<range.min or wavenumber>range.max will always be 0.
     */
    [[nodiscard]] range_t<wavenumber_t> wavenumber_range() const noexcept override {
        return { dist.range().min / u::mm, dist.range().max / u::mm };
    }
    
    /**
     * @brief Returns the mean wavenumber for this spectrum.
     */
    [[nodiscard]] wavenumber_t mean_wavenumber() const noexcept override { return wavenumber_range().centre(); }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] f_t f(const wavenumber_t wavenumber) const noexcept override {
        return wavenumber_range().contains(wavenumber) ? avg_spectral_pwr : 0;
    }
    
public:
    static std::unique_ptr<spectrum_real_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
