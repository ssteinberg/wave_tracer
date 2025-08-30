/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include <ranges>

#include <wt/util/gui/impl/perf_stat.hpp>
#include <wt/util/gui/impl/impl.hpp>
#include <wt/util/gui/imgui/utils.hpp>
#include <wt/util/gui/imgui/imgui_log.hpp>

#include <wt/util/statistics_collector/stat_collector_registry.hpp>
#include <wt/util/statistics_collector/stat_histogram.hpp>
#include <wt/util/statistics_collector/stat_timings.hpp>
#include <wt/util/statistics_collector/stat_counter_event.hpp>

#include <wt/util/logger/termcolor.hpp>
#include <wt/util/format/utils.hpp>


using namespace wt;
using namespace wt::gui;


std::vector<perf_stat_t> wt::gui::build_perf_stats() noexcept {
    std::vector<perf_stat_t> stats;

    const auto& collectors = stats::stat_collector_registry_t::instance().get_collectors();
    for (const auto& c : collectors) {
        if (const auto* histogram = dynamic_cast<const stats::stat_histogram_generic_t*>(c.get()); histogram) {
            if (histogram->is_empty()) continue;

            // histogram stat, create plot
            stats.emplace_back(c->name);
            stats.back().plot = std::make_unique<perf_stat_t::plot_type>(histogram->name, *histogram);
        }
        else {
            if (c->is_empty()) continue;

            std::stringstream ss;
            logger::termcolour::set_colourized(ss);
            ss << *c;
            auto str = std::move(ss).str();

            stats.emplace_back(c->name);
            auto& stat = stats.back();

            // break up into chunks, and ignore first (label) chunk
            for (const auto& chunkv : str | std::views::split('\t') | std::views::drop(1)) {
                auto chunk = format::trim(std::string{ chunkv.begin(), chunkv.end() }, "❙ \t\v\r\n");
                std::string lbl;

                // extract labels, they are formatted as 'dark white'
                static constexpr std::string lbl_prefix = "\033[2m\033[37m";
                if (chunk.starts_with(lbl_prefix)) {
                    static constexpr auto prefix_sz = lbl_prefix.length();
                    auto i = prefix_sz;
                    for (;i<chunk.length() && chunk[i]!='\033';++i) {}
                    if (i<chunk.length()) {
                        lbl = format::trim(chunk.substr(prefix_sz, i-prefix_sz-1));
                        chunk = chunk.substr(i);
                    }
                }
                else {
                    const auto empty_spaces = chunk.find("\033[1m ");
                    if (empty_spaces!=std::string::npos) {
                        auto i=1;
                        for (;chunk[empty_spaces+4+i]==' ';++i) {}
                        chunk.erase(empty_spaces+4,i);
                    }
                }

                stat.data.emplace_back(std::move(lbl),std::move(chunk));
            }

            // provide better labels for fields
            if (!!dynamic_cast<const stats::stat_timings_t*>(c.get())) {
                if (stat.data.size()==3) {
                    stat.data[0].first = "throughput";
                    stat.data[2].first = "range";
                        std::erase_if(stat.data[2].second, [](const auto c) { return c=='(' || c==')'; });
                }
            }
            else if (!!dynamic_cast<const stats::stat_counter_event_generic_t*>(c.get())) {
                stat.data[0].first = "events";
            }
        }
    }

    return stats;
}

inline auto begin_spanning_row() noexcept {
    const auto* table = ImGui::GetCurrentTable();

    // span all columns
    const ImRect row_r(table->WorkRect.Min.x, table->BgClipRect.Min.y, table->WorkRect.Max.x, table->RowPosY2);
    ImGui::PushClipRect(table->BgClipRect.Min, table->BgClipRect.Max, false);
}
inline auto end_spanning_row() noexcept {
    ImGui::PopClipRect();
}

inline void draw_imgui_table_node_graphic(const impl_t* pimpl, const perf_stat_t& node, int node_flags) noexcept {
    assert(node.plot);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    begin_spanning_row();

    const auto hist_plot_w = ImGui::GetCurrentTable()->WorkRect.GetWidth() - ImGui::GetCursorPosX();
    const auto hist_plot_h = m::min(175.f,hist_plot_w*3/5);
    const auto size = ImVec2{ hist_plot_w, hist_plot_h };
    plot_graph(size, *node.plot, pimpl->mono_font, ImPlotFlags_CanvasOnly);

    end_spanning_row();
}

inline void draw_imgui_table_node_data(const impl_t* pimpl, const perf_stat_t& node, int node_flags) noexcept {
    // write chunks
    for (const auto& d : node.data) {
        bool spanning_row = d.first.empty();

        ImGui::PushID((const void*)&d);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        const auto flags = node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        ImGui::TreeNodeEx(d.first.c_str(), flags);
        ImGui::PopID();

        if (spanning_row) {
            begin_spanning_row();
            ImGui::SameLine();
        }
        else
            ImGui::TableNextColumn();

        ImGui::PushFont(pimpl->mono_font, ImGui::GetFontSize());
        ImGui::TextAnsiUnformatted(&*d.second.begin(),&*d.second.end());
        ImGui::PopFont();

        if (spanning_row)
            end_spanning_row();
    }
}

void perf_stat_t::draw_imgui_table_node(const impl_t* pimpl, int node_flags) const noexcept {
    ImGui::PushID(this);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    begin_spanning_row();
    const bool open = ImGui::TreeNodeEx(name.c_str(), node_flags);
    end_spanning_row();

    if (open) {
        if (plot) draw_imgui_table_node_graphic(pimpl, *this, node_flags);
        else      draw_imgui_table_node_data(pimpl, *this, node_flags);

        ImGui::TreePop();
    }

    ImGui::PopID();
}
