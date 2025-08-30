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
#include <functional>

#include <wt/sampler/sampler.hpp>

#include <wt/math/common.hpp>
#include <wt/math/distribution/uniform_distribution.hpp>

#include <wt/util/unique_function.hpp>

#include "spectrum.hpp"

namespace wt::spectrum {

/**
 * @brief Analytic (real-valued) spectrum: uses an arbitrary used-supplied real-valued function of wavenumber.
 *        Does not provide an underlying distribution, distribution() returns ``nullptr``.
 *        Does not provide power() queries, which always return 0.
 */
class analytic_t final : public spectrum_real_t {
private:
    range_t<wavenumber_t> range;
    unique_function<f_t(wavenumber_t) const noexcept> func;
    std::string func_description;

public:
    analytic_t(std::string id, 
               range_t<wavenumber_t> range,
               std::regular_invocable<wavenumber_t> auto f,
               std::string func_description)
        : spectrum_real_t(std::move(id)),
          range(range), func(std::move(f)), func_description(std::move(func_description))
    {}
    analytic_t(analytic_t&&) = default;

    /**
     * @brief Returns the underlying spectrum distribution.
     */
    [[nodiscard]] const distribution1d_t* distribution() const noexcept override {
        // not supported
        assert(false);
        return nullptr;
    }

    /**
     * @brief Returns the total spectral power contained in this spectrum.
     *        Not support for ``analytic`` spectra.
     */
    [[nodiscard]] f_t power() const noexcept override {
        // not supported
        assert(false);
        return 0;
    }

    /**
     * @brief Returns the spectral power over the provided wavenumber range.
     *        Not support for ``analytic`` spectra.
     */
    [[nodiscard]] f_t power(const range_t<wavenumber_t>& wavenumbers) const noexcept override {
        // not supported
        assert(false);
        return 0;
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
    [[nodiscard]] wavenumber_t mean_wavenumber() const noexcept override { return range.centre(); }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] f_t f(const wavenumber_t wavenumber) const noexcept override {
        return range.contains(wavenumber) ? func(wavenumber) : 0;
    }
    
public:
    [[nodiscard]] scene::element::info_t description() const override;
};

}
