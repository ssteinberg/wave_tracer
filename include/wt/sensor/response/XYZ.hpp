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

#include <wt/spectrum/spectrum.hpp>

#include <wt/scene/element/scene_element.hpp>
#include <wt/wt_context.hpp>
#include <wt/math/common.hpp>

#include <wt/sensor/response/response.hpp>
#include <wt/sensor/response/tonemap/tonemap.hpp>
#include <wt/sensor/response/multichannel.hpp>

namespace wt::sensor::response {

class XYZ_t final : public response_t {
private:
    multichannel_t response;

public:
    XYZ_t(std::string id,
          const wt::wt_context_t &context);
    XYZ_t(XYZ_t&&) = default;
    virtual ~XYZ_t() noexcept = default;

    /**
     * @brief Returns the response function's pixel layout
     */
    [[nodiscard]] bitmap::pixel_layout_t pixel_layout() const noexcept override {
        return bitmap::pixel_layout_t{ 3 };
    }

    /**
     * @brief Returns the channel spectrum
     */
    [[nodiscard]] const auto* channel(std::uint16_t channel) const noexcept {
        return response.channel(channel);
    }
    
    /**
     * @brief Evaluates the sensor sensitivity function
     */
    [[nodiscard]] const f_t f(
            std::uint32_t channel, 
            const wavenumber_t& k) const noexcept override {
        return response.f(channel, k);
    }
    
    /**
     * @brief Returns the sensor's sensitivity spectrum (the sum of all channels' response spectra)
     */
    [[nodiscard]] const spectrum::spectrum_real_t& sensitivity() const noexcept override { return response.sensitivity(); }

public:
    static std::unique_ptr<response_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
