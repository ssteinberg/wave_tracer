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

#include <wt/wt_context.hpp>

namespace wt::spectrum {

/**
 * @brief Generic spectrum: a complex-valued function of wavenumber.
 *        Wavenumber \f$ k \f$ is related to wavelength \f$ \lambda \f$ via \f$ k = \frac{2\pi}{\lambda} \f$.
 */
class spectrum_t : public scene::scene_element_t {
public:
    static constexpr std::string scene_element_class() noexcept { return "spectrum"; }

public:
    spectrum_t(std::string id) : scene_element_t(std::move(id)) {}
    spectrum_t(const spectrum_t&) = default;
    spectrum_t(spectrum_t&&) = default;
    virtual ~spectrum_t() noexcept = default;
    
    /**
     * @brief Returns the range of wavenumbers for which this spectrum is defined.
     *        Querying the spectrum with wavenumber<range.min or wavenumber>range.max will always be 0.
     */
    [[nodiscard]] virtual range_t<wavenumber_t> wavenumber_range() const noexcept = 0;
    
    /**
     * @brief Query the spectrum. Returns the spectral value for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] virtual c_t value(const wavenumber_t wavenumber) const noexcept = 0;

public:
    static std::unique_ptr<spectrum_t> load(
            std::string id, 
            scene::loader::loader_t* loader, 
            const scene::loader::node_t& node, 
            const wt::wt_context_t &context);

    [[nodiscard]] virtual scene::element::info_t description() const override = 0;
};

/**
 * @brief Purely real-valued spectrum, for power and power-like distributions.
 */
class spectrum_real_t : public spectrum_t {
public:
    spectrum_real_t(std::string id) : spectrum_t(std::move(id)) {}
    spectrum_real_t(const spectrum_real_t&) = default;
    spectrum_real_t(spectrum_real_t&&) = default;
    virtual ~spectrum_real_t() noexcept = default;

    /**
     * @brief Returns the underlying spectrum distribution.
     */
    [[nodiscard]] virtual const distribution1d_t* distribution() const noexcept = 0;

    /**
     * @brief Returns the total spectral power contained in this spectrum.
     */
    [[nodiscard]] virtual f_t power() const noexcept = 0;

    /**
     * @brief Returns the spectral power over the provided wavenumber range.
     */
    [[nodiscard]] virtual f_t power(const range_t<wavenumber_t>& wavenumbers) const noexcept = 0;
    
    /**
     * @brief Returns the mean wavenumber for this spectrum.
     */
    [[nodiscard]] virtual wavenumber_t mean_wavenumber() const noexcept = 0;
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] virtual f_t f(const wavenumber_t wavenumber) const noexcept = 0;
    
    /**
     * @brief Query the spectrum. Returns the spectral value for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] c_t value(const wavenumber_t wavenumber) const noexcept final {
        return std::real(f(wavenumber));
    }
};

}
