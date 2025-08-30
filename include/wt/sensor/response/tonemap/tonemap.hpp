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
#include <optional>

#include <wt/scene/element/scene_element.hpp>
#include <wt/wt_context.hpp>

#include <wt/util/concepts.hpp>
#include <wt/util/unique_function.hpp>
#include <wt/util/math_expression.hpp>
#include <wt/math/common.hpp>
#include <wt/math/range.hpp>

#include <wt/bitmap/bitmap.hpp>

#include <wt/math/glm.hpp>
#include <glm/gtx/color_space.hpp>
#include "wt/bitmap/common.hpp"

#define TINYCOLORMAP_WITH_GLM
#include <tinycolormap.hpp>

namespace wt::sensor::response {

/**
 * @brief Tonemapping operator.
 */
enum class tonemap_e : std::uint8_t {
    /** @brief Passthrough (no tonemap).
     */
    linear,
    /** @brief Gamma correction, \f$ f(x) = x^{1/\gamma} \f$ for a provided exponent \f$ \gamma \f$.
     */
    gamma,
    /** @brief sRGB gamma correction.
     */
    sRGB,
    /** @brief Logarithmic mapping in decibels.
     */
    dB,
    /** @brief User-supplied arbitrary function.
     */
    function,
};

/**
 * @brief Mode of operation of the tonemapping operator.
 */
enum class tonemap_mode_e : std::uint8_t {
    /** @brief Switches between ``colourmap`` node when outout is monochromatic, and ``normal`` mode when output is polychromatic.
     */
    select,
    /** @brief Applies the tonemapping function per channel independently.
     */
    normal,
    /** @brief Applies the tonemapping function to monochromatic (greyscale) output.
     *         3-channel output is assumed to be linear RGB and is first converted to greyscale via colourspace::luminance().
     */
    colourmap,
};

/**
 * @brief Tonemap operators apply a postprocessing map, such as gamma correction, to the final film output.
 *        See `tonemap_e` and `tonemap_mode_e`.
 */
class tonemap_t : public scene::scene_element_t {
public:
    static constexpr std::string scene_element_class() noexcept { return "tonemap"; }

    static constexpr auto default_colourmap = tinycolormap::ColormapType::Magma;

private:
    tonemap_mode_e mode;
    tonemap_e func;
    std::optional<compiled_math_expression_t> user_func;
    std::string colourmap_name;

    union {
        f_t gamma;
        range_t<> db_range;
    };

    unique_function<vec3_t(f_t) const noexcept> function_mono;
    unique_function<vec3_t(vec3_t) const noexcept> function_poly;

    [[nodiscard]] inline f_t eval_user_func(const f_t& val) const {
        // set vars
        std::vector<f_t> vars;
        vars.resize(1);
        vars[0] = val;

        return user_func->eval(vars);
    }

public:
    tonemap_t(std::string id,
              tonemap_mode_e mode,
              tonemap_e func,
              tinycolormap::ColormapType colourmap,
              std::optional<compiled_math_expression_t> user_func,
              const f_t gamma,
              const range_t<>& db_range);

    tonemap_t(std::string id)
        : tonemap_t(std::move(id),
                    tonemap_mode_e::select,
                    tonemap_e::linear,
                    {}, {}, {}, {})
    {}

    /**
     * @brief Returns the tonemapping mode.
     */
    [[nodiscard]] inline auto get_tonemapping_mode() const noexcept { return mode; }
    /**
     * @brief Returns the tonemapping operator.
     */
    [[nodiscard]] inline auto get_tonemapping_op() const noexcept { return func; }

    /**
     * @brief Returns the colour encoding of a tonemapped result.
     *        Returns `linear` encoding for `linear` and `function` tonemapping operators, otherwise returns `sRGB` encoding.
     */
    [[nodiscard]] inline bitmap::colour_encoding_t get_colour_encoding() const noexcept {
        return func == tonemap_e::linear || func == tonemap_e::function ?
            bitmap::colour_encoding_type_e::linear :
            bitmap::colour_encoding_type_e::sRGB;
    }

    /**
     * @brief Returns the tonemapping operator's gamma. Only relevant to ``tonemap_e::gamma`` operators.
     */
    [[nodiscard]] inline auto get_gamma() const noexcept { return gamma; }
    /**
     * @brief Returns the tonemapping operator's dB range. Only relevant to ``tonemap_e::dB`` operators.
     */
    [[nodiscard]] inline auto get_dB_range() const noexcept { return db_range; }

    /**
     * @brief Returns the name of the colourmap. Only relevant when a colour-mapping mode is in use.
     */
    [[nodiscard]] inline const auto& get_colourmap_name() const noexcept { return colourmap_name; }

    /**
     * @brief Applies the tonemapping operator to a scalar value.
     */
    [[nodiscard]] inline auto operator()(f_t value) const noexcept {
        return function_mono(value);
    }
    /**
     * @brief Applies the tonemapping operator to an RGB triplet.
     */
    [[nodiscard]] inline auto operator()(vec3_t value) const noexcept {
        return function_poly(value);
    }

    /**
     * @brief Applies the tonemapping operator to a bitmap. ``bmp`` must be a single channel or RGB image, with its components implicitly convertible to ``f_t`` floating points.
     */
    template <FloatingPoint Fp>
    [[nodiscard]] inline auto operator()(const bitmap::bitmap_t<Fp>& bmp) const {
        const auto chnls = bmp.components();
        if (chnls!=1 && chnls!=3)
            throw std::runtime_error("(tonemap_t) invalid bitmap input to tonemapping operator");

        auto out = bitmap::bitmap_t<Fp>::create(bmp.width(), bmp.height(), bitmap::pixel_layout_e::RGB);
        for (auto p=0ul; p<bmp.total_pixels(); ++p) {
            if (chnls==1)
                reinterpret_cast<vec3_t*>(out.data())[p] = (*this)(bmp.data()[p]);
            if (chnls==3)
                reinterpret_cast<vec3_t*>(out.data())[p] = 
                    (*this)(vec3_t{ bmp.data()[3*p+0], bmp.data()[3*p+1], bmp.data()[3*p+2] });
        }

        return out;
    }

    static std::unique_ptr<tonemap_t> create_linear(
            std::string id, 
            tonemap_mode_e mode = tonemap_mode_e::normal,
            tinycolormap::ColormapType colourmap = default_colourmap) noexcept {
        return std::make_unique<tonemap_t>(std::move(id), mode, tonemap_e::linear, colourmap, std::nullopt, 0, range_t<>{});
    }
    static std::unique_ptr<tonemap_t> create_gamma(
            std::string id, 
            f_t gamma,
            tonemap_mode_e mode = tonemap_mode_e::normal,
            tinycolormap::ColormapType colourmap = default_colourmap) noexcept {
        return std::make_unique<tonemap_t>(std::move(id), mode, tonemap_e::gamma, colourmap, std::nullopt, gamma, range_t<>{});
    }
    static std::unique_ptr<tonemap_t> create_sRGB(
            std::string id, 
            tonemap_mode_e mode = tonemap_mode_e::normal,
            tinycolormap::ColormapType colourmap = default_colourmap) noexcept {
        return std::make_unique<tonemap_t>(std::move(id), mode, tonemap_e::sRGB, colourmap, std::nullopt, 0, range_t<>{});
    }
    static std::unique_ptr<tonemap_t> create_dB(
            std::string id, 
            const range_t<>& db_range,
            tinycolormap::ColormapType colourmap) noexcept {
        return std::make_unique<tonemap_t>(std::move(id), tonemap_mode_e::colourmap, tonemap_e::dB, colourmap, std::nullopt, 0, db_range);
    }
    static std::unique_ptr<tonemap_t> create_function(
            std::string id, 
            compiled_math_expression_t func,
            tonemap_mode_e mode = tonemap_mode_e::normal,
            tinycolormap::ColormapType colourmap = default_colourmap) noexcept {
        return std::make_unique<tonemap_t>(std::move(id), mode, tonemap_e::function, colourmap, std::move(func), 0, range_t<>{});
    }

public:
    static std::unique_ptr<tonemap_t> load(
            std::string id, 
            scene::loader::loader_t* loader, 
            const scene::loader::node_t& node, 
            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
