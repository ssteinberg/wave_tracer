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
#include <wt/sensor/response/XYZ.hpp>

#include <wt/spectrum/colourspace/RGB/RGB.hpp>
#include <wt/spectrum/colourspace/whitepoint.hpp>

namespace wt::sensor::response {

/**
 * @brief Multi-channel RGB response function.
 *        Internally, converts spectral data to XYZ and then to RGB. Conversion depends on the desired RGB colourspace and white point. Defaults to CIE colourspace with a D50 white point.
 */
class RGB_t final : public response_t {
public:
    static constexpr auto default_colourspace = colourspace::RGB_colourspace_e::CIE;
    static constexpr auto default_white_point = colourspace::white_point_e::D50;

private:
    XYZ_t XYZ;

    colourspace::RGB_colourspace_e colourspace;
    colourspace::white_point_e whitepoint;

    mat3_t conversion;

public:
    RGB_t(std::string id, 
          const wt::wt_context_t &context,
          std::shared_ptr<tonemap_t> tonemap,
          colourspace::RGB_colourspace_e colourspace,
          colourspace::white_point_e whitepoint)
        : response_t(std::move(id), std::move(tonemap)),
          XYZ(get_id() + "_XYZ", context),
          colourspace(colourspace),
          whitepoint(whitepoint),
          conversion(colourspace::conversion_matrix_XYZ_to_RGB(colourspace, whitepoint))
    {}
    RGB_t(RGB_t&&) = default;
    virtual ~RGB_t() noexcept = default;

    /**
     * @brief Returns the XYZ-to-RGB conversion matrix used by this RGB response function
     */
    [[nodiscard]] inline const auto& get_XYZ_to_RGB_matrix() const noexcept {
        return conversion;
    }

    /**
     * @brief Returns the colourspace of this RGB response function
     */
    [[nodiscard]] inline auto get_RGB_colourspace() const noexcept {
        return colourspace;
    }

    /**
     * @brief Returns the white point of this RGB response function
     */
    [[nodiscard]] inline auto get_whitepoint() const noexcept {
        return whitepoint;
    }

    /**
     * @brief Returns the response function's pixel layout
     */
    [[nodiscard]] bitmap::pixel_layout_t pixel_layout() const noexcept override {
        return bitmap::pixel_layout_e::RGB;
    }

    /**
     * @brief Evaluates the sensor sensitivity function
     */
    [[nodiscard]] const f_t f(
            std::uint32_t channel, 
            const wavenumber_t& k) const noexcept override;
    
    /**
     * @brief Returns the sensor's sensitivity spectrum (the sum of all channels' response spectra)
     */
    [[nodiscard]] const spectrum::spectrum_real_t& sensitivity() const noexcept override { return XYZ.sensitivity(); }

public:
    static std::unique_ptr<response_t> load(std::string id, 
                                            scene::loader::loader_t* loader, 
                                            const scene::loader::node_t& node, 
                                            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
