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
#include <map>

#include <wt/sampler/sampler.hpp>

#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

#include "spectrum.hpp"

namespace wt::spectrum {

/**
 * @brief A composition of one or more (real-valued) spectra, each defined over a distinct spectral range.
 *        Spectral ranges must not overlap.
 */
class composite_t final : public spectrum_real_t {
private:
    struct comparator_rangew_t {
        bool operator()(const auto& a, const auto& b) const {
            return a.max<=b.min;
        }
    };

    using map_range_t = range_t<wavenumber_t, range_inclusiveness_e::left_inclusive>;
    using map_t = std::map<map_range_t, 
                           std::shared_ptr<spectrum_real_t>, 
                           comparator_rangew_t>;

private:
    map_t spectra;
    range_t<wavenumber_t> range;
    wavenumber_t mean;
    f_t pwr;

public:
    composite_t(std::string id,
                map_t spectra)
        : spectrum_real_t(std::move(id)),
          spectra(std::move(spectra))
    {
        this->range = range_t<wavenumber_t>::null();
        f_t spectral_power = 0;
        wavenumber_t mean = 0/u::mm;
        for (const auto& s : this->spectra) {
            assert((this->range & s.first).empty());    // overlapping spectra?
            this->range |= s.first;

            const auto p = s.second->power(range_t<wavenumber_t>{ s.first.min,s.first.max });

            spectral_power += p;
            mean += s.second->mean_wavenumber() * p;
        }

        this->pwr = spectral_power;
        this->mean = spectral_power>0 ? mean / spectral_power : 0/u::mm;
    }
    composite_t(const composite_t&) = default;
    composite_t(composite_t&&) = default;

    /**
     * @brief Returns the underlying spectrum distribution.
     */
    [[nodiscard]] const distribution1d_t* distribution() const noexcept override {
        return nullptr;
    }

    /**
     * @brief Returns the total spectral power contained in this spectrum.
     */
    [[nodiscard]] f_t power() const noexcept override {
        return pwr;
    }

    /**
     * @brief Returns the spectral power over the provided wavenumber range.
     */
    [[nodiscard]] f_t power(const range_t<wavenumber_t>& wavenumbers) const noexcept override {
        f_t spectral_power = 0;
        for (const auto& s : this->spectra) {
            const auto range = wavenumbers & s.first;
            spectral_power += s.second->power(range);
        }

        return spectral_power;
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
        return mean;
    }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] f_t f(const wavenumber_t wavenumber) const noexcept override {
        const auto it = spectra.lower_bound(map_range_t::range(wavenumber));
        if (it!=spectra.end() && it->first.contains(wavenumber))
            return it->second->f(wavenumber);
        return 0;
    }
    
public:
    static std::unique_ptr<spectrum_real_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
