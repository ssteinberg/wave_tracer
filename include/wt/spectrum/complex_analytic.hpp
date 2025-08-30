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

#include <wt/scene/element/scene_element.hpp>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>

#include <wt/util/unique_function.hpp>

#include <wt/spectrum/spectrum.hpp>
#include <wt/wt_context.hpp>

namespace wt::spectrum {

/**
 * @brief Analytic (complex-valued) spectrum: uses an arbitrary used-supplied complex-valued function of wavenumber.
 *        Does not provide an underlying distribution_t, nor power() queries.
 */
class complex_analytic_t final : public spectrum_t {
private:
    range_t<wavenumber_t> range;
    unique_function<c_t(wavenumber_t) const noexcept> func;
    std::string func_description;

public:
    complex_analytic_t(std::string id, 
                       range_t<wavenumber_t> range,
                       std::regular_invocable<wavenumber_t> auto f,
                       std::string func_description)
        : spectrum_t(std::move(id)),
          range(range), func(std::move(f)), func_description(std::move(func_description))
    {}
    complex_analytic_t(complex_analytic_t&&) = default;

    /**
     * @brief Returns the range of wavenumbers for which this spectrum is defined.
     *        Querying the spectrum with wavenumber<range.min or wavenumber>range.max will always be 0.
     */
    [[nodiscard]] range_t<wavenumber_t> wavenumber_range() const noexcept override {
        return range;
    }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] c_t value(const wavenumber_t wavenumber) const noexcept override {
        return range.contains(wavenumber) ? func(wavenumber) : 0;
    }
    
public:
    [[nodiscard]] scene::element::info_t description() const override;
};

}
