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

#include <wt/interaction/polarimetric/stokes.hpp>
#include <wt/spectrum/spectrum.hpp>

#include <wt/scene/element/scene_element.hpp>
#include <wt/wt_context.hpp>
#include <wt/math/common.hpp>

#include <wt/sensor/response/response.hpp>
#include <wt/sensor/response/tonemap/tonemap.hpp>

namespace wt::sensor::response {

/**
 * @brief Single-channel sensor response function.
 */
class monochromatic_t final : public response_t {
private:
    std::shared_ptr<spectrum::spectrum_real_t> spectrum;

public:
    monochromatic_t(std::string id, 
                    std::shared_ptr<tonemap_t> tonemap,
                    std::shared_ptr<spectrum::spectrum_real_t> spectrum);
    monochromatic_t(const monochromatic_t&) = default;
    monochromatic_t(monochromatic_t&&) = default;
    virtual ~monochromatic_t() noexcept = default;

    /**
     * @brief Returns the response function's pixel layout
     */
    [[nodiscard]] bitmap::pixel_layout_t pixel_layout() const noexcept override {
        return bitmap::pixel_layout_e::L;
    }
    
    /**
     * @brief Evaluates the sensor sensitivity function
     */
    [[nodiscard]] const f_t f(
            std::uint32_t channel,
            const wavenumber_t& k) const noexcept override {
        assert(channel==0);
        return channel==0 ? spectrum->f(k) : 0;
    }
    
    /**
     * @brief Returns the sensor's sensitivity spectrum (the sum of all channels' response spectra)
     */
    [[nodiscard]] const spectrum::spectrum_real_t& sensitivity() const noexcept override { return *spectrum; }

public:
    static std::unique_ptr<response_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
