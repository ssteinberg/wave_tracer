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
#include <vector>
#include <memory>
#include <functional>

#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

#include <wt/util/unique_function.hpp>

#include <wt/spectrum/spectrum.hpp>

namespace wt::spectrum {

/**
 * @brief (Real-valued) spectrum that is an (arbitrary) function of several nested spectra.
 */
class function_t final : public spectrum_real_t {
public:
    using spectra_container_t = std::vector<std::shared_ptr<spectrum_real_t>>;
    using func_t = unique_function<f_t(
            const spectra_container_t&,
            const wavenumber_t     // k
        ) const noexcept>;

private:
    func_t func;
    std::string func_description;

    spectra_container_t spectra;

    [[nodiscard]] inline f_t eval_func(const Wavenumber auto k) const {
        return func(spectra, k);
    }

public:
    function_t(std::string id,
               spectra_container_t&& spectra,
               func_t&& func,
               std::string func_description)
        : spectrum_real_t(std::move(id)),
          func(std::move(func)),
          func_description(std::move(func_description)),
          spectra(std::move(spectra))
    {}
    function_t(std::string id,
               func_t&& func,
               std::string func_description)
        : function_t(std::move(id), {}, std::move(func), std::move(func_description))
    {}
    function_t(function_t&&) = default;
    virtual ~function_t() noexcept = default;

    /**
     * @brief Returns the underlying spectrum distribution.
     */
    [[nodiscard]] const distribution1d_t* distribution() const noexcept override {
        // not implemented
        return nullptr;
    }

    /**
     * @brief Returns the total spectral power contained in this spectrum.
     */
    [[nodiscard]] f_t power() const noexcept override {
        // not implemented
        return {};
    };

    /**
     * @brief Returns the spectral power over the provided wavenumber range.
     */
    [[nodiscard]] f_t power(const range_t<wavenumber_t>& wavenumbers) const noexcept override {
        // not implemented
        return {};
    }

    /**
     * @brief Returns the range of wavenumbers for which this spectrum is defined.
     *        Querying the spectrum with wavenumber<range.min or wavenumber>range.max will always be 0.
     */
    [[nodiscard]] range_t<wavenumber_t> wavenumber_range() const noexcept override {
        auto range = range_t<wavenumber_t>::null();
        for (const auto& s : spectra)
            range |= s->wavenumber_range();
        return range;
    }
    
    /**
     * @brief Returns the mean wavenumber for this spectrum.
     */
    [[nodiscard]] wavenumber_t mean_wavenumber() const noexcept override {
        // not implemented
        return {};
    }
    
    /**
     * @brief Query the spectrum. Returns the spectral power for the given wavenumber 'wavenumber'.
     */
    [[nodiscard]] f_t f(const wavenumber_t wavenumber) const noexcept override {
        return eval_func(wavenumber);
    }
    
public:
    static std::unique_ptr<spectrum_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
