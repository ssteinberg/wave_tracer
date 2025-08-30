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
#include <vector>

#include <wt/scene/element/info.hpp>
#include <wt/sensor/sensor.hpp>

#include <wt/util/gui/utils.hpp>

namespace wt::gui {

struct impl_t;

struct scene_info_t {
    std::string name,data,id;
    std::vector<std::unique_ptr<scene_info_t>> children;

    using plot_type = spectral_plot_t<512,3>;
    std::unique_ptr<plot_type> plot;
    std::unique_ptr<gl_image_t> image;

    // addresses used for imgui id, values irrelevant.
    void *popupid, *buttonid;
    bool popup_open = false;
    std::string popup_lbl;

    void draw_imgui_table_node(const impl_t* pimpl, int node_flags = 0) noexcept;
};

/**
 * @brief Builds scene info node from a scene_element description.
 *        std::nullopt for name indicates that class should be used instead.
 *
 * @param sensor sensor that is used for wavelength ranges on all spectral plots
 */
std::unique_ptr<scene_info_t> build_scene_info(
        const std::string_view prefix,
        std::optional<std::string> name,
        const scene::element::info_t& info,
        const sensor::sensor_t& sensor) noexcept;

}
