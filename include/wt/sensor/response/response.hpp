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

#include <wt/bitmap/bitmap.hpp>

#include <wt/sensor/response/tonemap/tonemap.hpp>

namespace wt::sensor::response {

/**
 * @brief Sensor's response function. This is responsible to transform spectral samples to film pixel data.
 *        f() converts spectral samples are converted to power values using the sensor's sensitivity spectrum for each channel.
 *        An optional tonemapping operator, get_tonemap(), transforms the final multichannel data using some mapping, e.g., gamma correction.
 */
class response_t : public scene::scene_element_t {
public:
    static constexpr std::string scene_element_class() noexcept { return "response"; }

private:
    std::shared_ptr<tonemap_t> tonemap;

public:
    response_t(std::string id,
               std::shared_ptr<tonemap_t>&& tonemap)
        : scene_element_t(std::move(id)),
          tonemap(std::move(tonemap))
    {}
    response_t(const response_t&) = default;
    response_t(response_t&&) = default;
    virtual ~response_t() noexcept = default;

    [[nodiscard]] const auto& get_tonemap() const noexcept {
        return tonemap;
    }

    /**
     * @brief Returns the sensor's pixel layout
     */
    [[nodiscard]] virtual bitmap::pixel_layout_t pixel_layout() const noexcept = 0;

    /**
     * @brief Returns number of channels in the sensor's pixel layout.
     */
    [[nodiscard]] auto channels() const noexcept { return pixel_layout().components; }
    
    /**
     * @brief Evaluates the sensor sensitivity function
     */
    [[nodiscard]] virtual const f_t f(
            std::uint32_t channel,
            const wavenumber_t& k) const noexcept = 0;
    
    /**
     * @brief Returns the sensor's sensitivity spectrum (the sum of all channels' response spectra)
     */
    [[nodiscard]] virtual const spectrum::spectrum_real_t& sensitivity() const noexcept = 0;

public:
    static std::unique_ptr<response_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);
};

}
