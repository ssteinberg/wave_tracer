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
#include <wt/math/range.hpp>
#include <wt/wt_context.hpp>

#include <wt/spectrum/colourspace/blackbody.hpp>
#include "piecewise_linear.hpp"

namespace wt::spectrum {

/**
 * @brief Blackbody spectrum. Underneath, uses a tightly-sampled piece-wise linear distribution, enabling sampling support.
 */
class blackbody_t final : public spectrum_real_t {
private:
    piecewise_linear_t spectrum;
    temperature_t T;

public:
    blackbody_t(std::string id, const temperature_t T, const range_t<wavelength_t>& wl_range, const f_t scale);
    blackbody_t(const blackbody_t&) = default;
    blackbody_t(blackbody_t&&) = default;

    /**
     * @brief Temperature in Kelvin of the blackbody radiator.
     */
    [[nodiscard]] inline auto temperature() const noexcept { return T; }
    /**
    * @brief Computes the Planckian locus in XYZ colourspace, i.e. the colour of a blackbody radiator.
    * @return vec3_t XYZ tristimulus colour
    */
    [[nodiscard]] inline auto locus_XYZ() const noexcept { return colourspace::planckian_locus(temperature()); }

    /**
     * @brief Returns the underlying spectrum distribution.
     */
    [[nodiscard]] const distribution1d_t* distribution() const noexcept override {
        return spectrum.distribution();
    }

    /**
     * @brief Returns the total spectral power contained in this spectrum.
     */
    [[nodiscard]] f_t power() const noexcept override {
        return spectrum.power();
    }

    /**
     * @brief Returns the spectral power over the provided wavenumber range.
     */
    [[nodiscard]] f_t power(const range_t<wavenumber_t>& wavenumbers) const noexcept override {
        return spectrum.power(wavenumbers);
    }

    /**
     * @brief Returns the range of wavenumbers for which this spectrum is defined.
     *        Querying the spectrum with wavenumber<range.min or wavenumber>range.max will always be 0.
     */
    [[nodiscard]] range_t<wavenumber_t> wavenumber_range() const noexcept override { return spectrum.wavenumber_range(); }
    
    /**
     * @brief Returns the mean wavenumber for this spectrum.
     */
    [[nodiscard]] wavenumber_t mean_wavenumber() const noexcept override { return spectrum.mean_wavenumber(); }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] f_t f(const wavenumber_t wavenumber) const noexcept override { return spectrum.f(wavenumber); }
    
public:
    static std::unique_ptr<spectrum_real_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
