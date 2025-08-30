/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <memory>

#include <wt/util/gui/dependencies.hpp>

#include <wt/util/gui/impl/common.hpp>
#include <wt/util/gui/impl/impl.hpp>

#include <wt/util/gui/utils.hpp>
#include <wt/util/gui/imgui/utils.hpp>
#include <wt/util/gui/imgui/imgui_log.hpp>
#include <wt/util/gui/imgui_knobs/imgui-knobs.h>
#include <wt/util/gui/gui.hpp>

#include <wt/util/format/chrono.hpp>

#include <wt/interaction/polarimetric/mueller.hpp>

#include <wt/math/common.hpp>
#include <wt/bitmap/bitmap.hpp>

#include <wt/version.hpp>


using namespace wt;


namespace wt::gui {


void logbox(impl_t* pimpl) {
    if (!pimpl->show_logbox)
        return;

    ImGui::SetNextWindowSize(ImVec2(0, ImGui::GetFontSize() * 12.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(logbox_imgui_lbl, nullptr, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::End();
        return;
    }

    constexpr auto verbosity_combo_width = 175;
    constexpr auto padding_end = 20;
    constexpr auto padding_verbosity = 60;
    const auto checkboxes_w = 
        ImGui::CalcTextSize("cout").x + ImGui::CalcTextSize("warnings").x + ImGui::CalcTextSize("errors").x + 
        3 * (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y*2);


    // log filter checkboxes
    ImGui::SameLine(ImGui::GetWindowWidth() - verbosity_combo_width - padding_end - checkboxes_w - padding_verbosity);

    ImGui::Checkbox("cout", &pimpl->logbox.sout_enabled[log_type_e::cout]);

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4{ .8,.9,.2,1});
    ImGui::Checkbox("warnings", &pimpl->logbox.sout_enabled[log_type_e::cwarn]);
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4{ 1,.5,.2,1});
    ImGui::Checkbox("errors", &pimpl->logbox.sout_enabled[log_type_e::cerr]);
    ImGui::PopStyleColor();

    // log verbosity select
    ImGui::SameLine(ImGui::GetWindowWidth() - verbosity_combo_width - padding_end);
    ImGui::PushItemWidth(verbosity_combo_width);

    static constexpr std::array<verbosity_e, 5> vlist = {
        verbosity_e::quiet, verbosity_e::important, verbosity_e::normal, verbosity_e::info, verbosity_e::debug
    };
    static constexpr std::array<const char*, 5> vlist_names = {
        "quiet", "important", "normal", "info", "debug"
    };
    int verbosity = 0;
    for (auto i=0ul; i<vlist.size(); ++i) if (vlist[i]==pimpl->loglevel) verbosity = i;
    if (ImGui::Combo("##logverbosity", &verbosity, vlist_names.data(), vlist_names.size()))
        pimpl->set_sout_verbosity(vlist[verbosity]);
    imgui_hover_tooltip("changes verbosity of log messages.\ndoes not apply retroactively.");

    ImGui::PopItemWidth();


    // log box
    ImGui::BeginChild("##logtext", ImVec2(0,0),
                      ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);

    const bool scroll_at_bottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
    const bool have_new_content = pimpl->sout.size()>pimpl->seen_sout_lines &&
                                  !scroll_at_bottom;

    ImGuiListClipper clipper;
    clipper.Begin(pimpl->sout.size());
    std::size_t line_offset{};
    while (clipper.Step())
        for (int line_no = clipper.DisplayStart; line_no<clipper.DisplayEnd && line_no+line_offset<pimpl->sout.size();) {
            if (!pimpl->logbox.sout_enabled[pimpl->sout[line_no+line_offset].first]) {
                ++line_offset;
                continue;
            }
            const auto& line = pimpl->sout[line_no+line_offset].second;
            ImGui::TextAnsiUnformatted(&*line.begin(), &*line.end());
            ++line_no;
        }

    if (scroll_at_bottom || pimpl->should_scroll_log_to_bottom) {
        ImGui::SetScrollHereY(1.0f);
        pimpl->seen_sout_lines = pimpl->sout.size();
        pimpl->should_scroll_log_to_bottom = false;
    }

    ImGui::EndChild();

    // new log items indicator
    if (have_new_content) {
        const auto btn_size = ImVec2{ 25,25 };
        const auto padding_size = ImVec2{ 8,8 };

        ImGui::SetCursorPos(ImGui::GetCurrentWindow()->ContentSize - btn_size - padding_size);
        ImGui::BeginChild("##scrollbtn_child", btn_size);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ .8,.9,1,1 });
        if (ImGui::Button("\uf103##logscrolldown", btn_size)) {
            pimpl->should_scroll_log_to_bottom = true;
        }
        ImGui::PopStyleColor();
        
        ImGui::EndChild();
    }
    
    ImGui::End();
}

void status_bar(impl_t* pimpl) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const auto cs = ImGui::GetContentRegionAvail();

    const auto padding = ImGui::GetStyle().ItemSpacing.x+2;
    const auto wdgh = ImGui::GetFontSize() + style.FramePadding.y*2;
    const auto btnw = wdgh;
    const auto paddingy = (pimpl->status_bar_height() - wdgh)/2;


    // sidebar / logbox toggle buttons

    ImGui::SetCursorPos({ padding/2,paddingy });
    const char* sidebar_toggle_id = pimpl->show_sidebar ? "\uf0d9##togglesidebar" : "\uf0da##togglesidebar";
    if (ImGui::Button(sidebar_toggle_id, ImVec2{ btnw,wdgh }))
        pimpl->show_sidebar = !pimpl->show_sidebar;

    ImGui::SameLine(cs.x-padding-btnw, 0);
    const char* logbox_toggle_id = pimpl->show_logbox ? "\uf0d7##togglelogbox" : "\uf0d8##togglelogbox";
    if (ImGui::Button(logbox_toggle_id, ImVec2{ btnw,wdgh }))
        pimpl->show_logbox = !pimpl->show_logbox;


    // status and progress

    if (pimpl->state != gui_state_t::loading && pimpl->has_rendering_started()) {
        const auto rstatus = pimpl->rendering_status();
        const bool done = rstatus.state == rendering_state_t::completed_successfully;
        const bool paused = rstatus.state == rendering_state_t::paused;
        const bool pausing = rstatus.state == rendering_state_t::pausing;
        const auto& progress = rstatus.progress();
        const auto& elapsed_time = rstatus.elapsed_rendering_time;
        const auto eta = rstatus.estimated_remaining_rendering_time();

        const auto remaining_time = eta - elapsed_time;
        const auto& [elapsed_days,elapsed_hours] = format::chrono::extract_duration<std::chrono::days>(elapsed_time);
        const auto& [remaining_days,remaining_hours] = format::chrono::extract_duration<std::chrono::days>(remaining_time);

        const auto& elapsed = 
            (elapsed_days>0 ? std::format("{} days ", elapsed_days) : "") + 
            std::format("{:%H:%M:%S}",floor<std::chrono::seconds>(elapsed_hours));
        std::string remaining = "";
        if (!done && rstatus.state == rendering_state_t::rendering && progress>0)
            remaining = (remaining_days>0 ? std::format("{} days ", remaining_days) : "") + 
                        std::format("{:%H:%M:%S}",floor<std::chrono::seconds>(remaining_hours));

        const std::string status_txt = 
            done ? "done" : 
            paused ? "paused" :
            pausing ? "pausing" :
            "rendering";
        const std::string elapsed_lbl = "elapsed";
        const std::string remaining_lbl = "remaining";
        const auto pb_txt = std::format("{:.1f}%", progress*100.f);
        const bool has_hover = rstatus.start_time>scene::time_point_t{};

        const auto pb_txt_colour     = ImVec4{ 1,1,1,1 };
        const auto status_txt_colour = 
            done ?    ImVec4{ .75,1,1,1 } :
            paused ?  ImVec4{ 1,.35,.2,1 } :
            pausing ? ImVec4{ 1,.65,.05,1 } :
            ImVec4{ 1,1,0,1 };
        const auto elapsed_colour   = ImVec4{ 0,.7,.7,1 };
        const auto remaining_colour = ImVec4{ .85,1,0,1 };
        const auto pcol = done ? elapsed_colour : ImVec4{ .25,.55,.6,1 };

        const auto pb_font_size = ImGui::GetFontSize()*1.025f;
        const auto txt_font_size = pb_font_size*.95;
        const auto lbl_font_size = pb_font_size*.6;

        const auto status_text_sz    = text_size(status_txt, pb_font_size);
        const auto pb_text_sz        = text_size(pb_txt, pb_font_size, pimpl->mono_font);
        const auto elapsed_text_sz   = text_size(elapsed, txt_font_size, pimpl->mono_font);
        const auto remaining_text_sz = text_size(remaining, txt_font_size, pimpl->mono_font);
        const auto elapsed_lbl_sz    = text_size(elapsed_lbl, lbl_font_size);
        const auto remaining_lbl_sz  = text_size(remaining_lbl, lbl_font_size);

        const auto offset = ImVec2{ 0,-1 };
        const auto lbl_padding_y = -1.f;
        const auto elapsed_y   = m::max(.0f,(cs.y-elapsed_text_sz.y-lbl_padding_y-elapsed_lbl_sz.y)/2);
        const auto remaining_y = m::max(.0f,(cs.y-remaining_text_sz.y-lbl_padding_y-remaining_lbl_sz.y)/2);

        constexpr auto pbw = 625;
        const auto width_pb = pbw + 2*padding;
        const auto width_with_texts =
            width_pb + 2*m::max(elapsed_text_sz.x,remaining_text_sz.x,elapsed_lbl_sz.x,remaining_lbl_sz.x) + 2*padding;
        const auto available_width = cs.x - 2*btnw - 4*padding;
        const bool has_pb  = available_width>=width_pb;
        const bool has_txt = available_width>=width_with_texts;

        if (has_pb) {
            ImGui::SetCursorPos(cs/2 - ImVec2{ pbw,wdgh }/2 + offset);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4{ pcol.x,pcol.y,pcol.z,1 });
            ImGui::ProgressBar(progress, ImVec2{ pbw,wdgh }, "");
            ImGui::PopStyleColor();

            if (has_hover) {
                // progress bar hover tooltip
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    std::string str;
                    if (rstatus.total_blocks>0)
                        str = std::format("blocks (completed/total): {:L}/{:L} ({:.2f}%)\nin progress: {:L}",
                            rstatus.completed_blocks, rstatus.total_blocks, progress*100.f, rstatus.blocks_in_progress);
                    else    // indefinite progress
                        str = std::format("completed blocks: {:L}\nin progress: {:L}",
                            rstatus.completed_blocks, rstatus.blocks_in_progress);
                    ImGui::SetTooltip("%s", str.c_str());
                }
            }

            ImGui::PushFont(nullptr, pb_font_size);

            ImGui::SetCursorPos(ImVec2{ (cs.x+pbw)/2-padding-pb_text_sz.x, (cs.y-pb_text_sz.y)/2 } + offset);
            ImGui::PushStyleColor(ImGuiCol_Text, pb_txt_colour);
            ImGui::PushFont(pimpl->mono_font, ImGui::GetFontSize());
            ImGui::Text("%s",pb_txt.c_str());
            ImGui::PopFont();
            ImGui::PopStyleColor();

            ImGui::SetCursorPos(cs/2 - status_text_sz/2 + offset);
            ImGui::PushStyleColor(ImGuiCol_Text, status_txt_colour);
            ImGui::Text("%s",status_txt.c_str());
            ImGui::PopStyleColor();

            ImGui::PopFont();

            if (has_txt) {
                if (!remaining.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, remaining_colour);

                    ImGui::SetCursorPos(ImVec2{ (cs.x+pbw)/2+padding, remaining_y } + offset);
                    ImGui::PushFont(pimpl->mono_font, txt_font_size);
                    ImGui::Text("%s",remaining.c_str());
                    ImGui::PopFont();

                    ImGui::SetCursorPos(ImVec2{ (cs.x+pbw)/2+padding, remaining_y+remaining_text_sz.y+lbl_padding_y } + offset);
                    ImGui::PushFont(nullptr, lbl_font_size);
                    ImGui::Text("%s",remaining_lbl.c_str());
                    ImGui::PopFont();

                    ImGui::PopStyleColor();
                }

                ImGui::PushStyleColor(ImGuiCol_Text, elapsed_colour);

                ImGui::SetCursorPos(ImVec2{ (cs.x-pbw)/2-padding-elapsed_text_sz.x, elapsed_y } + offset);
                ImGui::PushFont(pimpl->mono_font, txt_font_size);
                ImGui::Text("%s",elapsed.c_str());
                ImGui::PopFont();

                ImGui::SetCursorPos(ImVec2{ (cs.x-pbw)/2-padding-elapsed_lbl_sz.x, elapsed_y+elapsed_text_sz.y+lbl_padding_y } + offset);
                ImGui::PushFont(nullptr, lbl_font_size);
                ImGui::Text("%s",elapsed_lbl.c_str());
                ImGui::PopFont();

                ImGui::PopStyleColor();
            }
        }
    }
}

void dockspace(impl_t* pimpl) {
    constexpr auto dockspace_flags =
        ImGuiDockNodeFlags_PassthruCentralNode |
        // (ImGuiDockNodeFlags)ImGuiDockNodeFlags_NoTabBar |
        ImGuiDockNodeFlags_AutoHideTabBar |
        ImGuiDockNodeFlags_NoCloseButton |
        ImGuiDockNodeFlags_NoWindowMenuButton |
        ImGuiDockNodeFlags_NoUndocking;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const auto status_bar_h = pimpl->status_bar_height();

    // dock space
    if (pimpl->io->ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        const auto& bgcol = ImVec4{ 0,0,0,0 };
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bgcol);
        ImGuiID dockspace_id = dockspace_over_viewport(0, viewport, dockspace_flags,
                                                       {}, { 0,status_bar_h  });
        ImGui::PopStyleColor();
        
        if (!pimpl->main_layout_configured) {
            pimpl->main_layout_configured = true;
            
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            ImGuiID dock_id_side;
            auto dock_id_main = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_NoUndocking);
            auto dock_id_previewer = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_NoUndocking);
            auto dock_id_log = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_NoUndocking);

            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left,
                                        .175f, &dock_id_side, &dock_id_main);
            ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Down,
                                        .2f, &dock_id_log, &dock_id_previewer);
            
            ImGui::DockBuilderDockWindow(previewer_imgui_lbl, dock_id_previewer);
            ImGui::DockBuilderDockWindow(sidebar_imgui_lbl, dock_id_side);
            ImGui::DockBuilderDockWindow(scene_info_imgui_lbl, dock_id_side);
            ImGui::DockBuilderDockWindow(perf_stats_imgui_lbl, dock_id_side);
            ImGui::DockBuilderDockWindow(logbox_imgui_lbl, dock_id_log);

            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    // status bar
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0,0 });
    ImGui::PushStyleColor(ImGuiCol_WindowBg, status_bar_bg_colour);

    ImGui::SetNextWindowPos(viewport->WorkPos + ImVec2{ 0, viewport->WorkSize.y - status_bar_h }, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{ viewport->WorkSize.x, status_bar_h }, ImGuiCond_Always);
    ImGui::Begin(statusbar_imgui_lbl, nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse);
    status_bar(pimpl);
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

void previewer(impl_t* pimpl) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2( 0,0 ));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    if (!ImGui::Begin(gui::previewer_imgui_lbl, nullptr, ImGuiWindowFlags_NoDecoration)) {
        ImGui::End();
        ImGui::PopStyleVar(2);
        return;
    }
    ImGui::PopStyleVar(2);

    const auto wnd = ImGui::GetCurrentWindow();

    const auto pm = pimpl->current_preview_mode();

    {
        using namespace ImGuiTexInspect;

        const auto inspector_border = 0;
        const auto size = wnd->WorkRect.GetSize() - ImVec2{ 2*inspector_border, 2*inspector_border };

        int flags = InspectorFlags_ShowWrap;
        if (!pimpl->preview_tooltips)
            flags |= InspectorFlags_NoTooltip;
        if (!pimpl->preview_annotations)
            flags |= InspectorFlags_NoGrid;

        // scalar or polarimetric inspector
        if (pimpl->is_polarimetric_preview())
            BeginInspectorPanel("inspector", pimpl->preview_gl_image_polarimetric,
                                flags, SizeExcludingBorder{ size },
                                pimpl->mono_font);
        else
            BeginInspectorPanel("inspector", pimpl->preview_gl_image,
                                flags, SizeExcludingBorder{ size },
                                pimpl->mono_font);

        if (pimpl->is_polarimetric_preview()) {
            vec4_t filter;
            switch (pimpl->pol_mode_id) {
            default:
            case 0: filter = { 1,0,0,0 }; CurrentInspector_SetPolFilterMode(); break;
            case 1: filter = { 0,1,0,0 }; CurrentInspector_SetPolFilterMode(); break;
            case 2: filter = { 0,0,1,0 }; CurrentInspector_SetPolFilterMode(); break;
            case 3: filter = { 0,0,0,1 }; CurrentInspector_SetPolFilterMode(); break;
            case 4: CurrentInspector_SetPolDOPMode(); break;
            case 5: CurrentInspector_SetPolDOLPMode(); break;
            case 6: CurrentInspector_SetPolDOCPMode(); break;
            case 7: CurrentInspector_SetPolLPDirMode(); break;
            case 8: CurrentInspector_SetPolErrorMode(); break;
            case 9:
                const auto LPangle = pimpl->pol_LP_filter_angle * u::ang::deg;
                const auto M = mueller_operator_t::linear_polarizer(LPangle);
                filter = m::transpose(M.matrix())[0];
                CurrentInspector_SetPolFilterMode();
                break;
            }
            CurrentInspector_SetStokesFilter(ImVec4{ (float)filter.x,(float)filter.y,(float)filter.z,(float)filter.w });
        }

        if (pimpl->scene_updated) {
            CurrentInspector_SetRGBResponse(pimpl->RGB_response_function);
        }

        switch (pm) {
        default:
        case preview_mode_e::linear:
            CurrentInspector_SetLinearMode(pimpl->exposure);
            break;
        case preview_mode_e::gamma:
            CurrentInspector_SetGammaMode(pimpl->srgb_gamma, pimpl->gamma, pimpl->exposure);
            break;
        case preview_mode_e::db:
            CurrentInspector_SetDBMode(pimpl->db_range.x, pimpl->db_range.y);
            CurrentInspector_SetColourmap(pimpl->colourmap_id);
            break;
        case preview_mode_e::fc:
            CurrentInspector_SetFCMode(pimpl->fc_min, pimpl->fc_max, pimpl->fc_channel);
            CurrentInspector_SetColourmap(pimpl->colourmap_id);
            break;
        }

        if (pimpl->should_recentre_image) {
            pimpl->should_recentre_image = false;
            CurrentInspector_Recenter();
        }
        if (pimpl->should_fit_image) {
            pimpl->should_fit_image = false;
            CurrentInspector_Fit();
        }
        if (pimpl->should_fill_image) {
            pimpl->should_fill_image = false;
            CurrentInspector_Fill();
        }

        if (pimpl->preview_annotations)
            DrawAnnotations();

        EndInspectorPanel();
    }

    // annotations: legend and preview info
    {
        const auto cs = wnd->WorkRect.GetSize();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ .85,.85,.85,1 });

        if (pimpl->has_preview()) {
            if (!pimpl->custom_fc() && !pimpl->mirrored_fc())
            if (pm == preview_mode_e::db || pm == preview_mode_e::fc) {
                ImGui::PushFont(pimpl->mono_font, 13);

                // draw colourmap bar legend
                const auto tick0 = pm == preview_mode_e::db ? 
                    std::format("{:.1f} dB", pimpl->db_range.x) : std::format("{:.3f}", pimpl->fc_min);
                const auto tick1 = pm == preview_mode_e::db ? 
                    std::format("{:.1f} dB", pimpl->db_range.y) : std::format("{:.3f}", pimpl->fc_max);
                const auto tick0_sz = ImGui::CalcTextSize(tick0.c_str());
                const auto tick1_sz = ImGui::CalcTextSize(tick1.c_str());
                const auto ticksy = m::max(tick0_sz.y,tick1_sz.y);

                const auto padding = ImGui::GetStyle().FramePadding.x/3;
                const auto padding_frame = ImGui::GetStyle().FramePadding.x*2/3;
                constexpr auto ticks_padding = 2;

                const auto bar_size = ImVec2{ previewer_colourmap_bar_legend_size.x,previewer_colourmap_bar_legend_size.y };
                const auto legend_size =
                    bar_size + 
                    ImVec2{ padding_frame,padding_frame }*2.f +
                    ImVec2{ tick0_sz.x/2 + tick1_sz.x/2, ticksy + ticks_padding };
                const auto legend_pos = ImVec2{ padding+1, cs.y - (padding+legend_size.y) };

                ImGui::SetCursorPos(legend_pos);
                ImGui::BeginChild("##preview_legend", legend_size, 0, ImGuiWindowFlags_NoBackground);

                const auto bar_pos = ImVec2{ padding_frame+tick0_sz.x/2, padding_frame };
                ImGui::SetCursorPos(bar_pos);
                ImGui::Image((ImTextureID)pimpl->colourmap_legend_bars[pimpl->colourmap_id], bar_size);

                ImGui::SetCursorPos(ImVec2{ padding_frame, padding_frame+bar_size.y+ticks_padding });
                ImGui::Text("%s", tick0.c_str());
                ImGui::SameLine(padding_frame + tick0_sz.x/2 + bar_size.x - tick1_sz.x/2);
                ImGui::Text("%s", tick1.c_str());

                ImGui::EndChild();

                ImGui::PopFont();
            }
        }

        if (pimpl->has_preview()) {
            const auto padding = ImGui::GetStyle().FramePadding.x;

            const auto imgw = pimpl->preview_gl_image ? pimpl->preview_gl_image.width : pimpl->preview_gl_image_polarimetric.width;
            const auto imgh = pimpl->preview_gl_image ? pimpl->preview_gl_image.height : pimpl->preview_gl_image_polarimetric.height;
            const auto suffix = pimpl->preview_gl_image ? "" : " (Stokes)";

            const auto imglbl =
                std::to_string(imgw) + "×" + std::to_string(imgh) + 
                " @ " + std::format("{:.1f}",pimpl->spe_completed) + "spp" +
                suffix;

            ImGui::PushFont(pimpl->mono_font, 15);

            const auto lblsz = ImGui::CalcTextSize(imglbl.c_str());
            ImGui::SetCursorPos(cs - ImVec2{ padding,padding } - lblsz);
            ImGui::BeginChild("##preview_info", lblsz, 0, ImGuiWindowFlags_NoBackground);
            ImGui::Text("%s", imglbl.c_str());
            ImGui::EndChild();

            ImGui::PopFont();

            {
                // zoom controls
                const auto padding = ImGui::GetStyle().FramePadding.x;
                const auto btnw = 30, btnh = 30;
                const auto btns_sz = ImVec2{ 3*btnw+2*padding,btnh };

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0,0,0,0 });

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1,1,1,.6 });
                auto btnhov = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
                btnhov.w = .1;
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnhov);
                auto btnact = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
                btnact.w = .3;
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnact);

                ImGui::SetCursorPos(cs - ImVec2{ padding,2*padding + lblsz.y } - btns_sz);
                ImGui::BeginChild("##preview_image_btns", btns_sz, 0, ImGuiWindowFlags_NoBackground);

                ImGui::SetCursorPos({ 0,0 });
                if (ImGui::Button("\uf065##fillpreview", ImVec2{ btnw,btnh })) {
                    pimpl->should_fill_image = true;
                }
                imgui_hover_tooltip("fill");

                ImGui::SameLine(0,padding);
                if (ImGui::Button("\uf066##fitpreview", ImVec2{ btnw,btnh })) {
                    pimpl->should_fit_image = true;
                }
                imgui_hover_tooltip("fit");

                ImGui::SameLine(0,padding);
                if (ImGui::Button("\ue4be##centrepreview", ImVec2{ btnw,btnh })) {
                    pimpl->should_recentre_image = true;
                }
                imgui_hover_tooltip("centre");

                ImGui::EndChild();
                
                ImGui::PopStyleColor(4);
            }
        }

        ImGui::PopStyleColor();
    }

    ImGui::End();
}

void scene_info(impl_t* pimpl) {
    if (!pimpl->show_sidebar)
        return;

    if (!ImGui::Begin(gui::scene_info_imgui_lbl) || !pimpl->scene_info || !pimpl->ads_info) {
        ImGui::End();
        return;
    }

    auto& scene_info = *pimpl->scene_info;
    auto& ads_info = *pimpl->ads_info;

    static ImGuiTableFlags table_flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_Hideable;
    static ImGuiTreeNodeFlags tree_node_flags= ImGuiTreeNodeFlags_DrawLinesFull;

    if (ImGui::BeginTable("##sceneinfotbl", 3, table_flags)) {
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("data", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoHeaderLabel);
        ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_DefaultHide);
        ImGui::TableHeadersRow();

        scene_info.draw_imgui_table_node(pimpl, tree_node_flags);
        ads_info.draw_imgui_table_node(pimpl, tree_node_flags);

        ImGui::EndTable();
    }

    ImGui::End();
}

void perf_stats(impl_t* pimpl) {
    if constexpr (!do_perf_stats)
        return;

    if (!pimpl->show_sidebar) {
        pimpl->perf_stats_open = false;
        return;
    }
    if (!ImGui::Begin(gui::perf_stats_imgui_lbl)) {
        ImGui::End();
        pimpl->perf_stats_open = false;
        return;
    }

    if (!pimpl->perf_stats_open) {
        // just switched to perf stats
        // update stats, if needed
        pimpl->update_perf_stats_if_stale();

        pimpl->perf_stats_open = true;
    }

    static ImGuiTableFlags table_flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable |ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
    static ImGuiTreeNodeFlags tree_node_flags= ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull;

    if (ImGui::BeginTable("##perfstattbl", 2, table_flags)) {
        ImGui::TableSetupColumn("entry", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("data", ImGuiTableColumnFlags_NoHide);
        ImGui::TableHeadersRow();

        for (const auto& s : pimpl->perf_stats)
            s.draw_imgui_table_node(pimpl, tree_node_flags);
        ImGui::EndTable();
    }

    ImGui::End();
}

void sidebar(impl_t* pimpl) {
    if (!pimpl->show_sidebar)
        return;

    const ImGuiStyle& style = ImGui::GetStyle();
    const auto pm = pimpl->current_preview_mode();

    if (!ImGui::Begin(gui::sidebar_imgui_lbl)) {
        ImGui::End();
        return;
    }

    const auto border = ImGui::GetStyle().WindowBorderSize;
    const auto cs = ImGui::GetCurrentWindow()->Size - ImVec2{ 2*border,2*border };
    const auto padding = ImGui::GetStyle().ItemSpacing.x+2;
    const auto paddingy = 14.f;

    const auto btnh = ImGui::GetFontSize()*1.2f + style.FramePadding.y*2;
    const auto icnbtnh = ImGui::GetFontSize() + style.FramePadding.y*2;
    const auto icnbtnw = icnbtnh;
    const auto selected_col = ImVec4(0.1f, 0.25f, 1.f, 1.0f);

    // preview mode buttons
    ImGui::Dummy(ImVec2(0, paddingy));
    if (ImGui::CollapsingHeader("preview mode", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Dummy(ImVec2(0, paddingy/2));

        const auto buttons = 4;
        const auto btnw = (cs.x - (buttons+1)*padding) / buttons;

        ImGui::PushStyleColor(ImGuiCol_Button,
            pm==preview_mode_e::linear ? selected_col : style.Colors[ImGuiCol_Button]);
        ImGui::SetCursorPosX(padding);
        if (ImGui::Button("linear##modelinear", ImVec2{ btnw,btnh })) {
            pimpl->set_mode_linear();
        }
        imgui_hover_tooltip("linear with adjustable exposure.\n\nkeyboard shortcut: 1");
        ImGui::PopStyleColor();

        ImGui::SameLine(0,padding);

        ImGui::PushStyleColor(ImGuiCol_Button,
            pm==preview_mode_e::gamma ? selected_col : style.Colors[ImGuiCol_Button]);
        if (ImGui::Button("gamma##modegamma", ImVec2{ btnw,btnh })) {
            pimpl->set_mode_gamma();
        }
        imgui_hover_tooltip("adjustable gamma correction and exposure.\n\nkeyboard shortcut: 2");
        ImGui::PopStyleColor();

        ImGui::SameLine(0,padding);

        ImGui::PushStyleColor(ImGuiCol_Button,
            pm==preview_mode_e::db ? selected_col : style.Colors[ImGuiCol_Button]);
        if (ImGui::Button("log##modedB", ImVec2{ btnw,btnh })) {
            pimpl->set_mode_db();
        }
        imgui_hover_tooltip("colour-coded decibel values.\n\nkeyboard shortcut: 3");
        ImGui::PopStyleColor();

        ImGui::SameLine(0,padding);

        ImGui::PushStyleColor(ImGuiCol_Button,
            pm==preview_mode_e::fc ? selected_col : style.Colors[ImGuiCol_Button]);
        if (ImGui::Button("FC##modeFC", ImVec2{ btnw,btnh })) {
            pimpl->set_mode_fc();
        }
        imgui_hover_tooltip("false colour channel preview.\n\nkeyboard shortcut: 4");
        ImGui::PopStyleColor();

        // preview mode controls

        if (pm != preview_mode_e::db && pm != preview_mode_e::fc) {
            // exposure
            ImGui::SetCursorPosX(padding);
            ImGui::Text(" exposure");

            float& e = pimpl->exposure;

            ImGui::SetCursorPosX(padding);
            if (ImGui::Button("−##exposureminus", ImVec2{ icnbtnw,icnbtnh }))
                pimpl->dec_exposure();

            ImGui::SameLine(0,padding);

            ImGui::PushItemWidth(cs.x-2*icnbtnw-4*padding);
            ImGui::PushFont(pimpl->mono_font, ImGui::GetFontSize());
            ImGui::SliderFloat("##exposure", &e, -5,+5);
            ImGui::PopFont();
            ImGui::PopItemWidth();
            imgui_hover_tooltip("exposure scales the image brightness by 2^e, where e is the exposure value.\n\nkeyboard shortcut: E and LSHIFT+E");

            ImGui::SameLine(0,padding);

            if (ImGui::Button("\u002b##exposureplus", ImVec2{ icnbtnw,icnbtnh }))
                pimpl->inc_exposure();
        }

        if (pm == preview_mode_e::gamma) {
            // gamma
            ImGui::BeginDisabled(pimpl->srgb_gamma);
            float& g = pimpl->gamma;

            ImGui::SetCursorPosX(padding);
            if (ImGui::Button("−##gammaminus", ImVec2{ icnbtnw,icnbtnh }))
                pimpl->dec_gamma();

            ImGui::SameLine(0,padding);

            ImGui::PushItemWidth(cs.x-2*icnbtnw-4*padding);
            ImGui::PushFont(pimpl->mono_font, ImGui::GetFontSize());
            ImGui::SliderFloat("##gamma", &g, .01,5);
            ImGui::PopFont();
            ImGui::PopItemWidth();
            imgui_hover_tooltip("gamma correction raises pixel values to the power of 1/gamma.\ngamma is applied after exposure.\n\nkeyboard shortcut: G and LSHIFT+G");

            ImGui::SameLine(0,padding);

            if (ImGui::Button("\u002b##gammaplus", ImVec2{ icnbtnw,icnbtnh }))
                pimpl->inc_gamma();

            g = m::max<float>(.01,g);
            ImGui::EndDisabled();

            ImGui::SetCursorPosX(padding);
            ImGui::Text(" gamma");

            // sRGB toggle
            ImGui::SameLine(0, 2*padding);
            ImGui::PushStyleColor(ImGuiCol_Button,
                pimpl->srgb_gamma ? selected_col : style.Colors[ImGuiCol_Button]);
            ImGui::Toggle("sRGB##gammamode_sRGB", &pimpl->srgb_gamma,
                          ImGuiToggleFlags_Animated, { 40,ImGui::GetTextLineHeight()+6 });
            imgui_hover_tooltip("use the sRGB colourspace gamma.\n\nkeyboard shortcut: LCTRL+G");
            ImGui::PopStyleColor();
        }

        if (pm == preview_mode_e::db) {
            // decibels
            ImGui::SetCursorPosX(padding);
            ImGui::Text("logarithmic (dB)");

            const auto txtw = ImGui::CalcTextSize("max").x;

            auto& dB = pimpl->db_range;

            ImGui::SetCursorPosX(padding);
            ImGui::Text("min");
            ImGui::SameLine(2*padding+txtw);

            if (ImGui::Button("−##dbmin_minus", ImVec2{ icnbtnw,icnbtnh }))
                dB.x -= .1;

            ImGui::SameLine(0,padding);

            ImGui::PushItemWidth(cs.x-2*icnbtnw-5*padding-txtw);
            ImGui::PushFont(pimpl->mono_font, ImGui::GetFontSize());
            ImGui::SliderFloat("##dBmin", &dB.x, -200,20);
            ImGui::PopFont();
            ImGui::PopItemWidth();
            imgui_hover_tooltip("range of colour-coded dB values.");

            ImGui::SameLine(0,padding);

            if (ImGui::Button("\u002b##dBmin_plus", ImVec2{ icnbtnw,icnbtnh }))
                dB.x += .1;

            dB.y = m::max(dB.x,dB.y);

            ImGui::SetCursorPosX(padding);
            ImGui::Text("max");
            ImGui::SameLine(2*padding+txtw);

            if (ImGui::Button("−##dbmax_minus", ImVec2{ icnbtnw,icnbtnh }))
                dB.y -= .1;

            ImGui::SameLine(0,padding);

            ImGui::PushItemWidth(cs.x-2*icnbtnw-5*padding-txtw);
            ImGui::PushFont(pimpl->mono_font, ImGui::GetFontSize());
            ImGui::SliderFloat("##dBmax", &dB.y, -200,20);
            ImGui::PopFont();
            ImGui::PopItemWidth();
            imgui_hover_tooltip("range of colour-coded dB values.");

            ImGui::SameLine(0,padding);

            if (ImGui::Button("\u002b##dBmax_plus", ImVec2{ icnbtnw,icnbtnh }))
                dB.y += .1;

            dB.x = m::min(dB.x,dB.y);
        }

        if (pm == preview_mode_e::fc) {
            // false colour
            ImGui::SetCursorPosX(padding);
            ImGui::Text("false colour");

            if (!pimpl->custom_fc()) {
                if (!pimpl->mirrored_fc()) {
                    pimpl->fc_max = m::max<float>(pimpl->fc_max,0);
                    pimpl->fc_min = m::clamp<float>(pimpl->fc_min,0,pimpl->fc_max);
                }

                ImGui::SetCursorPosX(padding);

                ImGui::PushItemWidth((cs.x-3*padding)/2);

                if (pimpl->mirrored_fc()) {
                    ImGui::BeginDisabled();
                    pimpl->fc_min = -pimpl->fc_max;
                }
                ImGui::PushFont(pimpl->mono_font, ImGui::GetFontSize());
                ImGui::SliderFloat("##fc_min", &pimpl->fc_min, pimpl->mirrored_fc() ? -1 : 0, pimpl->mirrored_fc() ? 0 : 1);
                ImGui::PopFont();
                imgui_hover_tooltip("lower range of values for false colour visualization.");
                if (pimpl->mirrored_fc())
                    ImGui::EndDisabled();

                pimpl->fc_max = m::max(pimpl->fc_max,pimpl->fc_min);
                ImGui::SameLine(0,padding);
                ImGui::PushFont(pimpl->mono_font, ImGui::GetFontSize());
                ImGui::SliderFloat("##fc_max", &pimpl->fc_max, 0, 1);
                ImGui::PopFont();
                imgui_hover_tooltip("upper range of values for false colour visualization.");
                pimpl->fc_min = m::min(pimpl->fc_max,pimpl->fc_min);

                ImGui::PopItemWidth();

                if (pimpl->is_rgb_preview()) {
                    ImGui::SetCursorPosX(padding);
                    const auto cmlblw = ImGui::CalcTextSize("colourmap").x;
                    static constexpr std::array<const char*, 5> vlist_names = {
                        "R (linear)", "G (linear)", "B (linear)", "luminance (linear)", "L٭ brightness"
                    };

                    const auto opts = pimpl->lock_linear_fc() ? vlist_names.size()-1 : vlist_names.size();
                    pimpl->fc_channel = m::min<int>(opts-1, pimpl->fc_channel);

                    ImGui::PushItemWidth(cs.x-3*padding-cmlblw);
                    ImGui::Combo("channel##fcchannel", &pimpl->fc_channel, vlist_names.data(), opts);
                }
            }
        }

        // colourmap
        if (!pimpl->custom_fc() && !pimpl->mirrored_fc())
        if (pm == preview_mode_e::db || pm == preview_mode_e::fc) {
            ImGui::SetCursorPosX(padding);
            const auto cmlblw = ImGui::CalcTextSize("colourmap").x;
            ImGui::PushItemWidth(cs.x-3*padding-cmlblw);
            ImGui::Combo("colourmap##colourmapid", &pimpl->colourmap_id,
                         colourmap_names.data(), colourmap_names.size());
        }

        // reset button
        ImGui::Dummy(ImVec2(0, paddingy/2));

        const auto resetbtnw = (cs.x - 4*padding) / 3 * 1.25f;
        ImGui::SetCursorPosX(cs.x-resetbtnw-padding);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1,.7,.6,1 });
        if (ImGui::Button("reset##resetcontrols", ImVec2{ resetbtnw,btnh })) {
            pimpl->reset_preview_controls();
        }
        ImGui::PopStyleColor();
        imgui_hover_tooltip("reset image controls.\n\nkeyboard shortcut: R");
    }


    // polarization preview controls
    if (pimpl->is_polarimetric_preview()) {
        ImGui::Dummy(ImVec2(0, paddingy));

        if (ImGui::CollapsingHeader("polarization", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SetCursorPosX(padding);
            const auto cmlblw = ImGui::CalcTextSize("mode").x;
            static constexpr std::array<const char*, 10> vlist_names = {
                "intensity", "S1", "S2", "S3",
                "DoP", "DoLP", "DoCP",
                "LP direction",
                "errors",
                "LP filter",
            };
            ImGui::PushItemWidth(cs.x-3*padding-cmlblw);
            ImGui::Combo("mode##pol_mode_id", &pimpl->pol_mode_id, vlist_names.data(), vlist_names.size());

            if (pimpl->pol_mode_id==9) {
                const auto knob_size = ImGui::GetTextLineHeight() * 4.f;

                ImGui::SetCursorPosX(padding);
                ImGui::Text("angle");

                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.2f, .65f, .85f, 1.0f));
                ImGui::SameLine(cs.x/2-padding-knob_size/2);
                ImGui::PushID("##LPangle");
                ImGuiKnobs::Knob("", &pimpl->pol_LP_filter_angle, -90.f, +90.f, 0, "%.1f°",
                                 ImGuiKnobVariant_WiperLine, knob_size,
                                 ImGuiKnobFlags_InputSameLine | ImGuiKnobFlags_NoTitle,
                                 10, m::pi, m::pi*2);
                ImGui::PopID();
                ImGui::PopStyleColor();
            }
        }
    }


    if constexpr (draw_histogram) {
    // image histogram
    ImGui::Dummy(ImVec2(0, paddingy));
    if (ImGui::CollapsingHeader("histogram", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {
        pimpl->histogram_shown = true;

        const auto padding = ImGui::GetStyle().ItemSpacing.x;
        const auto hist_plot_w = cs.x-2*padding;
        const auto hist_plot_h = m::min(hist_plot_w*4/7,150.f);
        const auto size = ImVec2{ hist_plot_w, hist_plot_h };
        
        if (pimpl->image_histogram && pimpl->image_histogram->max>pimpl->image_histogram->min) {
            ImGui::SetCursorPosX(padding);
            plot_graph(size, *pimpl->image_histogram, pimpl->mono_font);
        }
    } else {
        pimpl->histogram_shown = false;
    }
    }


    // rendering controls
    {
        const auto posy = ImGui::GetCursorPosY();
        const auto y = m::max(posy, cs.y-215);

        const auto btnpadding = 3*padding;
        constexpr auto btnscount = 3;
        const auto btnh = ImGui::GetFontSize()*1.5f + style.FramePadding.y*2;
        const auto btnw = (cs.x - (btnscount-1)*padding - 2*btnpadding) / (float)btnscount;

        ImGui::SetCursorPos({ 0,y });
        ImGui::Dummy(ImVec2(0, 2*paddingy));

        // preview annotations controls
        {
            ImGui::PushFont(nullptr, 17);

            const auto tglw = 40;
            const auto tglh = ImGui::GetFontSize()*1.f + style.FramePadding.y*2;
            const auto txt1_sz = ImGui::CalcTextSize("tooltips").x;
            const auto txt2_sz = ImGui::CalcTextSize("annotations").x;
            const auto sz = 2*tglw + txt1_sz + txt2_sz + 3*padding;

            ImGui::SetCursorPosX((cs.x-sz)/2);

            ImGui::PushStyleColor(ImGuiCol_Button,
                pimpl->preview_tooltips ? selected_col : style.Colors[ImGuiCol_Button]);
            ImGui::Toggle("tooltips##preview_tooltips", &pimpl->preview_tooltips,
                          ImGuiToggleFlags_Animated, { tglw,tglh });
            ImGui::PopStyleColor();

            ImGui::PopFont();

            imgui_hover_tooltip("show image tooltips.");

            ImGui::SameLine(0,3*padding);

            ImGui::PushFont(nullptr, 17);
            ImGui::PushStyleColor(ImGuiCol_Button,
                pimpl->preview_annotations ? selected_col : style.Colors[ImGuiCol_Button]);
            ImGui::Toggle("annotations##preview_annotations", &pimpl->preview_annotations,
                          ImGuiToggleFlags_Animated, { tglw,tglh });
            ImGui::PopStyleColor();
            ImGui::PopFont();

            imgui_hover_tooltip("draw image annotations when zoomed in.");
        }

        ImGui::Dummy(ImVec2(0, 2*paddingy));

        // buttons 
        bool can_resume = false, can_pause = false, can_capture = false;
        if (pimpl->has_rendering_started() && pimpl->state == gui::gui_state_t::rendering) {
            const auto state = pimpl->rendering_status().state;
            can_resume  = state == rendering_state_t::pausing || state == rendering_state_t::paused;
            can_pause   = state == rendering_state_t::rendering;
            can_capture = can_resume || can_pause;
        }

        ImGui::SetCursorPosX(btnpadding);

        ImGui::BeginDisabled(!can_resume);
        if (ImGui::Button("\uf04b##play", ImVec2{ btnw,btnh })) {
            pimpl->renderer_resume();
        }
        ImGui::EndDisabled();
        imgui_hover_tooltip("signals the renderer to resume processing.\n\nkeyboard shortcut: SPACE");

        ImGui::SameLine(0,padding);

        ImGui::BeginDisabled(!can_pause);
        if (ImGui::Button("\uf04c##pause", ImVec2{ btnw,btnh })) {
            pimpl->renderer_pause();
        }
        ImGui::EndDisabled();
        imgui_hover_tooltip("signals the renderer to pause.\nrendering will be paused once all the blocks that have been scheduled are completed.\n\nkeyboard shortcut: SPACE");

        ImGui::SameLine(0,padding);

        ImGui::BeginDisabled(!can_capture);
        if (ImGui::Button("\uf0c7##capture", ImVec2{ btnw,btnh })) {
            pimpl->capture_intermediate();
        }
        ImGui::EndDisabled();
        imgui_hover_tooltip("signals the renderer to capture an intermediate result.\nthe renderer will queue the remaining blocks for a non-partial render.\nonce all blocks are completed, an intermediate result will be saved to output directory.\n\nkeyboard shortcut: CTRL+S");
    }


    // bottom text and about button
    {
        const auto posy = ImGui::GetCursorPosY();
        const auto& vers_string = pimpl->wtversion_string;

        ImGui::PushFont(nullptr, 17);
        const auto sz = ImGui::CalcTextSize(vers_string.c_str());

        const auto pos = ImVec2{ (cs.x - sz.x)/2, cs.y-sz.y-paddingy };
        if (pos.y>=posy+paddingy) {
            if (pos.x>=2*padding+icnbtnw) {
                ImGui::SetCursorPos(pos);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ .4,.4,.4,1});
                ImGui::Text("%s",vers_string.c_str());
                ImGui::PopStyleColor();
            }
            ImGui::PopFont();

            ImGui::SetCursorPos({ cs.x-padding-icnbtnw, pos.y+.5f*sz.y-.5f*btnh });
            if (ImGui::Button("\u003f##aboutbtn", ImVec2{ icnbtnw,icnbtnh })) {
                pimpl->about_popup_open = true;
                ImGui::OpenPopup("about##aboutpopup");
            }
        } else {
            ImGui::PopFont();
        }
    }

    // about modal pop-up
    const auto wnd_y = 380;
    ImGui::SetNextWindowSize(ImVec2{ 380,wnd_y });
    if (ImGui::BeginPopupModal("about##aboutpopup", &pimpl->about_popup_open,
                               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
        const auto cs = ImGui::GetContentRegionAvail();

        const auto header1_colour = ImVec4{ .75,.85,1,1 };
        const auto header2_colour = ImVec4{ .85,.85,.85,1 };
        const auto* header1_text = "wave_tracer";
        const auto& header2_text = wt_version_t{}.full_version_string();
        const auto subheader_colour = ImVec4{ .75,.75,.75,1 };
        const auto* subheader_text1 = "by Shlomi Steinberg";
        const auto* subheader_text2 = "https://www.ssteinberg.xyz";

        constexpr auto icn_size = 140;
        
        ImGui::SetCursorPos(ImVec2{ (cs.x-icn_size)/2,50 });
        ImGui::Image((ImTextureID)pimpl->icon, ImVec2{ icn_size,icn_size });

        ImGui::Dummy(ImVec2(0, 10));

        ImGui::PushFont(nullptr, 30);
        const auto header1_text_sz = ImGui::CalcTextSize(header1_text);
        ImGui::SetCursorPosX((cs.x-header1_text_sz.x)/2);
        ImGui::PushStyleColor(ImGuiCol_Text, header1_colour);
        ImGui::Text("%s",header1_text);
        ImGui::PopFont();
        ImGui::PopStyleColor();

        ImGui::PushFont(nullptr, 20);
        const auto header2_text_sz = ImGui::CalcTextSize(header2_text.c_str());
        ImGui::SetCursorPosX((cs.x-header2_text_sz.x)/2);
        ImGui::PushStyleColor(ImGuiCol_Text, header2_colour);
        ImGui::Text("%s",header2_text.c_str());
        ImGui::PopFont();
        ImGui::PopStyleColor();

        ImGui::PushFont(nullptr, 15);
        const auto subheader_text1_sz = ImGui::CalcTextSize(subheader_text1);
        const auto subheader_text2_sz = ImGui::CalcTextSize(subheader_text2);

        ImGui::SetCursorPos({ (cs.x-subheader_text1_sz.x)/2,wnd_y-50 });
        ImGui::PushStyleColor(ImGuiCol_Text, subheader_colour);
        ImGui::Text("%s",subheader_text1);
        ImGui::PopStyleColor();

        ImGui::SetCursorPosX((cs.x-subheader_text2_sz.x)/2);
        ImGui::TextLinkOpenURL(subheader_text2);

        ImGui::PopFont();
        
        ImGui::EndPopup();
    }

    
    ImGui::End();
}

bool exit_popup(bool& triggered_done) {
    ImGui::SetNextWindowSize(ImVec2{ 210,130 });
    if (ImGui::BeginPopupModal("exit##exitdialog", nullptr,
                               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
        const auto cs = ImGui::GetContentRegionAvail();
        const auto btnw = 55, btnh=30;
        const auto spacing = 10;
        const auto padding = (cs.x - 2*btnw - spacing)/2;

        const char* txt = "confirm exit?";
        const auto sz = ImGui::CalcTextSize(txt).x;

        ImGui::Dummy({ 0,5 });
        ImGui::SetCursorPosX((cs.x-sz)/2);
        ImGui::Text("confirm exit?");

        ImGui::Dummy({ 0,15 });

        ImGui::SetCursorPosX(padding);
        if (ImGui::Button("no##exitno", { btnw,btnh })) {
            ImGui::CloseCurrentPopup();
            triggered_done = false;
        }
        ImGui::SameLine(0, spacing);
        if (ImGui::Button("yes##exityes", { btnw,btnh })) {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return true;
        }
        
        ImGui::EndPopup();
    }

    return false;
}

inline void loading_popup(const gui_t::bootstrap_data_t& bs) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, 0, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2{ 525,150 });

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 12,10 });
    // loading window with progress bars
    ImGui::Begin("##loading_popup", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleVar();

    ImGui::Dummy(ImVec2(0, 6));

    imgui_loading_progress_bar("Loading scene", vec3_t{ 0,.8,.8 },
                               bs.scene_loading_progress.load());

    imgui_loading_progress_bar("Loading resources", vec3_t{ .2,1,.2 },
                               bs.resource_loading_progress.load());

    imgui_loading_progress_bar("Constructing ADS", vec3_t{ .2,.2,1 },
                               bs.ads_construction_progress.load());
    if (const auto ads_status = bs.ads_construction_status.load(); ads_status)
        imgui_hover_tooltip(ads_status->c_str());

    ImGui::End();
}


}


gui_t::gui_t(wt_context_t& ctx,
             std::unique_ptr<gui_t::bootstrap_data_t> scene_bootstrapper,
             renderer_results_callback_t write_out_render_results)
    : context(ctx)
{
    auto pimpl = new gui::impl_t(
            ctx,
            std::move(scene_bootstrapper),
            std::move(write_out_render_results));
    this->ptr = std::shared_ptr<gui::impl_t>(pimpl);


    {
        pimpl->init(ctx);
        pimpl->load_fonts();
        // enable docking
        pimpl->io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }


    bool done = false;
    bool termination_triggered = false, termination_confirmed = false, termination_pending = false;

    // main loop
    while (!done) {
        // process window and input events
        {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL3_ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT)
                    termination_triggered = true;
                if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(pimpl->window))
                    termination_triggered = true;

                if (pimpl->state != gui::gui_state_t::loading) {
                    // handle UI shortcuts
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        if (event.key.key == SDLK_1) pimpl->set_mode_linear();
                        else if (event.key.key == SDLK_2) pimpl->set_mode_gamma();
                        else if (event.key.key == SDLK_3) pimpl->set_mode_db();
                        else if (event.key.key == SDLK_4) pimpl->set_mode_fc();
                        else if (event.key.key == SDLK_E)
                            (event.key.mod&SDL_KMOD_LSHIFT) ? pimpl->dec_exposure(5) : pimpl->inc_exposure(5);
                        else if (event.key.key == SDLK_G && (event.key.mod&SDL_KMOD_LCTRL))
                            pimpl->toggle_gamma_srgb();
                        else if (event.key.key == SDLK_G)
                            (event.key.mod&SDL_KMOD_LSHIFT) ? pimpl->dec_gamma(4) : pimpl->inc_gamma(4);
                        else if (event.key.key == SDLK_R) pimpl->reset_preview_controls();
                        else if (event.key.key == SDLK_F && (event.key.mod&SDL_KMOD_LSHIFT)) pimpl->should_recentre_image = true;
                        else if (event.key.key == SDLK_F && (event.key.mod&SDL_KMOD_LCTRL))  pimpl->should_fill_image = true;
                        else if (event.key.key == SDLK_F) pimpl->should_fit_image = true;
                        else if (event.key.key == SDLK_SPACE) {
                            if (pimpl->has_rendering_started() && pimpl->state == gui::gui_state_t::rendering)
                                pimpl->renderer_toggle_pauseresume();
                        }
                        else if (event.key.key == SDLK_S && (event.key.mod&SDL_KMOD_LCTRL))
                            pimpl->capture_intermediate();
                    }
                }
            }
        }

        // process status updates
        if (pimpl->state==gui::gui_state_t::loading) {
            if (termination_pending)
                done = true;

            // check scene loading status
            using namespace std::chrono_literals;
            if (pimpl->is_scene_loading_done() && !done) {
                pimpl->create_scene();
                pimpl->print_summary();

                // start rendering
                pimpl->state = gui::gui_state_t::rendering;
                pimpl->start_rendering(this);
            }
        }
        else {
            // check rendering process
            if (pimpl->state==gui::gui_state_t::rendering) {
                if (pimpl->is_scene_renderer_done()) {
                    pimpl->process_rendering_result();
                    pimpl->state=gui::gui_state_t::idle;
                }
            } else {
                if (termination_pending)
                    done = true;
            }

            // process preview updates
            pimpl->update_preview();
        }

        // background update
        if (SDL_GetWindowFlags(pimpl->window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        pimpl->new_frame();
        ImGui::PushFont(nullptr, 19);


        if (pimpl->state==gui::gui_state_t::loading) {
            // loading
            const auto* bs = pimpl->get_scene_bootstrapper().get();
            if (bs)
                gui::loading_popup(*bs);
        }
        else {
            gui::dockspace(pimpl);

            gui::logbox(pimpl);
            gui::sidebar(pimpl);
            gui::scene_info(pimpl);
            gui::perf_stats(pimpl);
            gui::previewer(pimpl);
        }

        // handle exit dialog
        if (termination_triggered && !termination_confirmed) {
            if (pimpl->state != gui::gui_state_t::rendering)
                termination_confirmed = true;
            else
                ImGui::OpenPopup("exit##exitdialog");
            termination_triggered = false;
        }
        if (gui::exit_popup(termination_triggered))
            termination_confirmed = true;
        // termination is confirmed either via SIGINTs or via exit dialog
        if (termination_confirmed) {
            if (pimpl->scene_renderer && pimpl->state==gui::gui_state_t::rendering)
                pimpl->scene_renderer->interrupt(std::make_unique<wt::scene::interrupts::terminate_t>());
            termination_pending = true;
        }


        ImGui::PopFont();

        // rendering
        pimpl->render();

        pimpl->scene_updated = false;
    }

    pimpl->deinit();
}


[[nodiscard]] bool gui_t::polarimetric_preview() const noexcept {
    return true;
}

void gui_t::update(const std::string& preview_id,
                   sensor::developed_scalar_film_t<2>&& surface,
                   const f_t spe_completed,
                   const sensor::response::tonemap_t* tonemap) const {
    auto pimpl = static_cast<gui::impl_t*>(ptr.get());
    pimpl->push_new_preview(std::make_shared<gui::preview_bitmap_t>(std::move(surface)),
                            spe_completed);
}

void gui_t::update(const std::string& preview_id,
                   sensor::developed_polarimetric_film_t<2>&& surface,
                   const f_t spe_completed,
                   const sensor::response::tonemap_t* tonemap) const {
    auto pimpl = static_cast<gui::impl_t*>(ptr.get());
    pimpl->push_new_preview(std::make_shared<gui::preview_bitmap_polarimetric_t>(std::move(surface)),
                            spe_completed);
}
