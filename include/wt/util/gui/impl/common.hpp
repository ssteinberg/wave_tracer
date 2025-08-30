/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/scene/scene_renderer.hpp>

#include <wt/util/gui/dependencies.hpp>
#include <wt/util/gui/utils.hpp>


namespace wt::gui {


using rendering_state_t = scene::rendering_state_t;


static constexpr bool draw_histogram = true;
static constexpr bool do_perf_stats = true;

constexpr auto default_sout_verbosity = verbosity_e::info;

constexpr auto initial_window_size_small = vec2u32_t{ 1024, 720 };
constexpr auto initial_window_size = vec2u32_t{ 1920, 1440 };
constexpr auto minimum_window_size = vec2u32_t{ 800, 600 };

constexpr auto previewer_colourmap_bar_legend_size = vec2u32_t{ 150,10 };

constexpr auto window_clear_colour = ImVec4{ .2, .3, .4, 1 };
constexpr auto status_bar_bg_colour = ImVec4{ 0.1098039224743843f *2/3, 0.1490196138620377f *2/3, 0.168627455830574f *2/3, 1.0f };

constexpr auto plot_col_mono       = ImVec4{ 1.,1.,1., .7 };
constexpr auto plot_col_mono_line  = ImVec4{ 1.,1.,1., 1 };
constexpr auto plot_col_twin1      = ImVec4{ .2,.2,1., .6 };
constexpr auto plot_col_twin1_line = ImVec4{ .2,.2,1., 1 };
constexpr auto plot_col_twin2      = ImVec4{ 1.,.2,.2, .6 };
constexpr auto plot_col_twin2_line = ImVec4{ 1.,.2,.2, 1 };
constexpr auto plot_col_rgb1       = ImVec4{ 1.,.3,.3, .55 };
constexpr auto plot_col_rgb1_line  = ImVec4{ 1.,.3,.3, 1 };
constexpr auto plot_col_rgb2       = ImVec4{ .3,1.,.3, .55 };
constexpr auto plot_col_rgb2_line  = ImVec4{ .3,1.,.3, 1 };
constexpr auto plot_col_rgb3       = ImVec4{ .2,.2,1., .55 };
constexpr auto plot_col_rgb3_line  = ImVec4{ .1,.1,1., 1 };

constexpr auto open_logbox_by_default = false;
constexpr auto open_sidebar_by_default = true;

inline const char* const statusbar_imgui_lbl = "##statusbar";
inline const char* const previewer_imgui_lbl = "##previewer";
inline const char* const logbox_imgui_lbl = "log##logbox";
inline const char* const sidebar_imgui_lbl = "preview##sidebar";
inline const char* const scene_info_imgui_lbl = "scene##sceneinfo";
inline const char* const perf_stats_imgui_lbl = "stats##perfstat";

static constexpr std::array<const char*, 6> colourmap_names = {
    "magma", "turbo", "inferno", "plasma", "viridis", "cividis"
};


}
