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

#include <wt/scene/element/scene_element.hpp>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>
#include <wt/math/distribution/distribution1d.hpp>

#include <wt/spectrum/spectrum.hpp>
#include <wt/wt_context.hpp>

namespace wt::spectrum {

/**
 * @brief Complex-valued spectrum. Uses a pair of (real-valued) spectra: one for the real part and one for the imaginary part.
 */
class complex_container_t final : public spectrum_t {
private:
    std::shared_ptr<spectrum_real_t> real_spectrum;
    std::shared_ptr<spectrum_real_t> imag_spectrum;

public:
    complex_container_t(
            std::string id,
            std::shared_ptr<spectrum_real_t> real,
            std::shared_ptr<spectrum_real_t> imag) 
        : spectrum_t(std::move(id)),
          real_spectrum(std::move(real)),
          imag_spectrum(std::move(imag))
    {
        assert(!!real_spectrum);
    }
    complex_container_t(const complex_container_t&) = default;
    complex_container_t(complex_container_t&&) = default;
    virtual ~complex_container_t() noexcept = default;
    
    /**
     * @brief Returns the range of wavenumbers for which this spectrum is defined.
     *        Querying the spectrum with wavenumber<range.min or wavenumber>range.max will always be 0.
     */
    [[nodiscard]] range_t<wavenumber_t> wavenumber_range() const noexcept override {
        if (!imag_spectrum) return real_spectrum->wavenumber_range();
        return real_spectrum->wavenumber_range() | imag_spectrum->wavenumber_range();
    }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] c_t value(const wavenumber_t wavenumber) const noexcept override {
        return c_t{
            real_spectrum->f(wavenumber),
            imag_spectrum ? imag_spectrum->f(wavenumber) : 0,
        };
    }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] inline c_t f(const wavenumber_t wavenumber) const noexcept {
        return value(wavenumber);
    }

public:
    static std::unique_ptr<spectrum_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
