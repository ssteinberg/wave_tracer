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
#include <utility>
#include <memory>
#include <vector>

#include <wt/util/gui/utils.hpp>

namespace wt::gui {

struct impl_t;

struct perf_stat_t {
    std::string name;
    std::vector<std::pair<std::string,std::string>> data;

    using plot_type = plot_t<std::uint32_t>;
    std::unique_ptr<plot_type> plot;

    void draw_imgui_table_node(const impl_t* pimpl, int node_flags = 0) const noexcept;
};

std::vector<perf_stat_t> build_perf_stats() noexcept;

}
