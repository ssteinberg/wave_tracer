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

#include <wt/spectrum/colourspace/RGB/RGB_to_spectral.hpp>
#include <wt/math/distribution/binned_piecewise_linear_distribution.hpp>

#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

#include "spectrum.hpp"

namespace wt::spectrum {

/**
 * @brief A (real-valued) spectrum that upsamples an RGB colour triplet to spectral data.
 *        Defined between wavelengths of 380nm and 780nm.
 *        See `wt::colourspace::RGB_to_spectral`.
 */
class rgb_t final : public spectrum_real_t {
public:
    static constexpr auto lambda_min = colourspace::RGB_to_spectral::min_lambda;
    static constexpr auto lambda_max = colourspace::RGB_to_spectral::max_lambda;

    /**
     * @brief Creates a binned piecewise-linear distribution from an RGB colour ``rgb``. Uses `wt::colourspace::RGB_to_spectral` for uplifting to spectral.
     */
    [[nodiscard]] static std::unique_ptr<binned_piecewise_linear_distribution_t> rgb_to_binned_linear_distribution(
            const vec3_t& rgb) noexcept;

private:
    vec3_t rgb;
    std::unique_ptr<binned_piecewise_linear_distribution_t> dist;

public:
    rgb_t(std::string id,
          const vec3_t& rgb)
        : spectrum_real_t(std::move(id)),
          rgb(rgb),
          dist(rgb_to_binned_linear_distribution(rgb))
    {}
    rgb_t(const rgb_t&) = delete;
    rgb_t(rgb_t&&) = default;

    /**
     * @brief Returns the underlying spectrum distribution.
     */
    [[nodiscard]] const distribution1d_t* distribution() const noexcept override {
        return dist.get();
    }

    /**
     * @brief Returns the total spectral power contained in this spectrum.
     */
    [[nodiscard]] f_t power() const noexcept override {
        return dist->total();
    };

    /**
     * @brief Returns the spectral power over the provided wavenumber range.
     */
    [[nodiscard]] f_t power(const range_t<wavenumber_t>& wavenumbers) const noexcept override {
        const auto p = dist->integrate(
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
        return {
            wavelen_to_wavenum(lambda_max),
            wavelen_to_wavenum(lambda_min)
        };
    }
    
    /**
     * @brief Returns the mean wavenumber for this spectrum.
     */
    [[nodiscard]] wavenumber_t mean_wavenumber() const noexcept override {
        return wavenumber_range().centre();
    }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] f_t f(const wavenumber_t wavenumber) const noexcept override {
        return colourspace::RGB_to_spectral::uplift(rgb, wavenum_to_wavelen(wavenumber));
    }
    
public:
    static std::unique_ptr<spectrum_real_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
