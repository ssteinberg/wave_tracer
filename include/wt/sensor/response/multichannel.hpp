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

#include <wt/interaction/polarimetric/stokes.hpp>
#include <wt/spectrum/spectrum.hpp>

#include <wt/scene/element/scene_element.hpp>
#include <wt/wt_context.hpp>
#include <wt/math/common.hpp>

#include <wt/sensor/response/response.hpp>

namespace wt::sensor::response {

/**
 * @brief Multi-channel sensor response function.
 *        Each channel is defined by its own sensitivity spectrum. The spectral ranges of different channels might overlap.
 */
class multichannel_t final : public response_t {
private:
    std::vector<std::shared_ptr<spectrum::spectrum_real_t>> channels;
    std::unique_ptr<spectrum::spectrum_real_t> sensitivity_spectrum;

public:
    multichannel_t(std::string id, 
                   std::vector<std::shared_ptr<spectrum::spectrum_real_t>> channels);
    multichannel_t(multichannel_t&&) = default;
    virtual ~multichannel_t() noexcept = default;

    /**
     * @brief Returns the response function's pixel layout
     */
    [[nodiscard]] bitmap::pixel_layout_t pixel_layout() const noexcept override {
        return bitmap::pixel_layout_t{ std::uint8_t(channels.size()) };
    }

    /**
     * @brief Returns the channel spectrum
     */
    [[nodiscard]] const auto* channel(std::uint16_t channel) const noexcept { return channels[channel].get(); }
    
    /**
     * @brief Evaluates the sensor sensitivity function
     */
    [[nodiscard]] const f_t f(
            std::uint32_t channel,
            const wavenumber_t& k) const noexcept override {
        assert(channel<channels.size());
        return channel<channels.size() ? channels[channel]->f(k) : 0;
    }
    
    /**
     * @brief Returns the sensor's sensitivity spectrum (the sum of all channels' response spectra)
     */
    [[nodiscard]] const spectrum::spectrum_real_t& sensitivity() const noexcept override { return *sensitivity_spectrum; }

public:
    static std::unique_ptr<response_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
