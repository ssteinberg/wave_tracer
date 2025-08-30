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
#include <wt/math/distribution/discrete_distribution.hpp>
#include <wt/wt_context.hpp>

#include "spectrum.hpp"

namespace wt::spectrum {

/**
 * @brief Discrete (real-valued) spectrum: a weighted sum of a finite count of Dirac deltas.
 */
class discrete_t final : public spectrum_real_t {
private:
    discrete_distribution_t<vec2_t> dist;

public:
    discrete_t(std::string id, discrete_distribution_t<vec2_t> dist)
        : spectrum_real_t(std::move(id)),
          dist(std::move(dist))
    {}
    discrete_t(const discrete_t&) = default;
    discrete_t(discrete_t&&) = default;

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
        f_t p=0;
        for (const auto& i : dist)
            p += wavenumbers.contains(i.x / u::mm) ? i.y : 0;
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
        wavenumber_t p = 0/u::mm;
        for (const auto& v : dist)
            p += v.x*v.y / u::mm;
        return p/dist.total();
    }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] f_t f(const wavenumber_t wavenumber) const noexcept override {
        return dist.total() * dist.pdf(u::to_inv_mm(wavenumber), measure_e::discrete);
    }

public:
    static std::unique_ptr<spectrum_real_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
