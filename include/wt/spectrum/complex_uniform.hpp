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

#include <wt/spectrum/spectrum.hpp>
#include <wt/wt_context.hpp>

namespace wt::spectrum {

/**
 * @brief Uniform (complex-valued) spectrum: returns a fixed complex constant for all wavenumbers within a range.
 *        Range may include all non-negative wavenumbers, \f$ k \in [0\ \textrm{mm}^{-1},+\infty\ \textrm{mm}^{-1}) \f$, or any subset.
 */
class complex_uniform_t final : public spectrum_t {
private:
    c_t val;
    range_t<wavenumber_t> krange;

public:
    complex_uniform_t(std::string id,
                      c_t val, 
                      const range_t<wavenumber_t> krange = range_t<wavenumber_t>::positive())
        : spectrum_t(std::move(id)),
          val(val),
          krange(krange)
    {}
    complex_uniform_t(const complex_uniform_t&) = default;
    complex_uniform_t(complex_uniform_t&&) = default;
    virtual ~complex_uniform_t() noexcept = default;
    
    /**
     * @brief Returns the range of wavenumbers for which this spectrum is defined.
     *        Querying the spectrum with wavenumber<range.min or wavenumber>range.max will always be 0.
     */
    [[nodiscard]] range_t<wavenumber_t> wavenumber_range() const noexcept override {
        return krange;
    }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] c_t value(const wavenumber_t wavenumber) const noexcept override {
        return krange.contains(wavenumber) ? val : c_t{ 0 };
    }

public:
    static std::unique_ptr<spectrum_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
