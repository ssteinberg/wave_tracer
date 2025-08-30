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

#include <wt/util/gui/impl/common.hpp>
#include <wt/util/gui/impl/scene_info.hpp>
#include <wt/util/gui/impl/impl.hpp>
#include <wt/util/gui/imgui/imgui.hpp>
#include <wt/util/gui/imgui/utils.hpp>

#include <wt/bsdf/bsdf.hpp>
#include <wt/sensor/response/response.hpp>
#include <wt/sensor/response/monochromatic.hpp>
#include <wt/spectrum/spectrum.hpp>
#include <wt/spectrum/discrete.hpp>
#include <wt/spectrum/uniform.hpp>
#include <wt/spectrum/complex_uniform.hpp>
#include <wt/spectrum/rgb.hpp>
#include <wt/texture/texture.hpp>
#include <wt/texture/bitmap.hpp>
#include <wt/texture/complex.hpp>
#include <wt/interaction/surface_profile/surface_profile.hpp>

#include <wt/scene/element/attributes.hpp>


using namespace wt;
using namespace wt::gui;


template <typename T>
inline auto gl_image_from_texture_storage(const bitmap::texture2d_storage_t& bmp_storage) {
    auto bitmap = bmp_storage.create_bitmap<T>();
    if (bitmap.components()<=2) {
        // convert to RGB
        bitmap = bitmap.template convert_texels<T>(bitmap::pixel_layout_e::RGB);
    }
    // handle wired sizes
    if (bitmap.dimensions().x%4!=0 || bitmap.dimensions().y%4!=0) {
        const auto newsize = (bitmap.dimensions() + vec2u32_t{ 3,3 }) / 4u * 4u;
        bitmap = bitmap.resize(newsize);
    }

    return std::make_unique<gl_image_t>(
        bitmap.data(), bitmap.width(), bitmap.height(),
        bitmap.components(), bitmap.component_bytes()
    );
}

inline std::unique_ptr<scene_info_t> scene_info_for_attribute(
        std::optional<std::string> name,
        const wt::scene::element::attribute_t* attrib,
        const sensor::sensor_t& sensor) {
    using namespace wt::scene::element;

    if (const auto se = dynamic_cast<const attributes::element_attribute_t*>(attrib); se) {
        // scene element
        const auto* element = se->get_element();
        assert(element);
        if (!element) return nullptr;

        std::string prefix;
        // choose prefix by type
        if (dynamic_cast<const ::wt::emitter::emitter_t*>(element))
            prefix = "\uf0eb";
        else if (dynamic_cast<const ::wt::sensor::sensor_t*>(element))
            prefix = "\uf06e";
        else if (dynamic_cast<const ::wt::bsdf::bsdf_t*>(element))
            prefix = "\uf0e7";
        else if (dynamic_cast<const ::wt::spectrum::spectrum_t*>(element))
            prefix = "\ue473";
        else if (dynamic_cast<const ::wt::texture::texture_t*>(element) ||
                 dynamic_cast<const ::wt::texture::complex_t*>(element))
            prefix = "\uf5fd";

        auto node = build_scene_info(prefix, std::move(name),element->description(), sensor);

        node->popup_lbl = node->name + " — " + node->id;

        // load gl image for 2d textures
        if (const auto* bmp = dynamic_cast<const ::wt::texture::bitmap_t*>(element); bmp) {
            const auto& bmp_storage = bmp->get_texture()->get_storage();
            if (bmp_storage.comp_size==1) 
                node->image = gl_image_from_texture_storage<std::uint8_t>(bmp_storage);
            else if (bmp_storage.comp_size==2) 
                node->image = gl_image_from_texture_storage<std::uint16_t>(bmp_storage);
            else if (bmp_storage.comp_size==4) 
                node->image = gl_image_from_texture_storage<float>(bmp_storage);
        }

        // build spectrum plot for spectra and sensor's sensitivity
        auto sensor_krange = sensor.sensitivity_spectrum().wavenumber_range();
        if (sensor_krange.length()==zero) {
            const auto expand = sensor_krange.min/f_t(20);
            sensor_krange = sensor_krange.grow(expand);
        }

        static std::size_t plot_count = 0;
        bool erase_bins_child = false;
        if (dynamic_cast<const ::wt::spectrum::uniform_t*>(element) || 
            dynamic_cast<const ::wt::spectrum::complex_uniform_t*>(element)) {
            // ignore constant spectra
        }
        else if (const auto* discrete = dynamic_cast<const ::wt::spectrum::discrete_t*>(element); discrete) {
            node->plot = std::make_unique<scene_info_t::plot_type>(
                    std::format("{}",plot_count++),
                    *discrete,
                    sensor_krange);
            // erase_bins_child = true;
        }
        else if (const auto* real_spectrum = dynamic_cast<const ::wt::spectrum::spectrum_real_t*>(element); real_spectrum) {
            node->plot = std::make_unique<scene_info_t::plot_type>(
                    std::format("{}",plot_count++),
                    *real_spectrum,
                    sensor_krange);
            erase_bins_child = true;
        }
        else if (const auto* complex_spectrum = dynamic_cast<const ::wt::spectrum::spectrum_t*>(element); complex_spectrum) {
            node->plot = std::make_unique<scene_info_t::plot_type>(
                    std::format("{}",plot_count++),
                    *complex_spectrum,
                    sensor_krange);
            erase_bins_child = true;
        }
        if (const auto* response = dynamic_cast<const ::wt::sensor::response::response_t*>(element); response) {
            const auto range = response->sensitivity().wavenumber_range();
            node->plot = std::make_unique<scene_info_t::plot_type>(
                    std::format("{}",plot_count++),
                    *response,
                    range);
            erase_bins_child = true;
        }

        if (erase_bins_child) {
            for (auto it=node->children.begin(); it!=node->children.end(); ++it) {
                if (it->get()->name == "bins") {
                    node->children.erase(it);
                    break;
                }
            }
        }

        return node;
    }

    // data attributes

    auto node = std::make_unique<scene_info_t>(name.value_or(""));

    // plain?
    if (const auto data = dynamic_cast<const attributes::data_attribute_t*>(attrib); data) {
        node->data = data->to_string();
        return node;
    }
    // container?
    else if (const auto container = dynamic_cast<const attributes::container_attribute_t*>(attrib); container) {
        if (const auto array = dynamic_cast<const attributes::array_t*>(attrib); array) {
            // array
            for (const auto& atr : *array)
                node->children.emplace_back(scene_info_for_attribute(std::nullopt, atr.get(), sensor));
        }
        else if (const auto map = dynamic_cast<const attributes::map_t*>(attrib); map) {
            // map
            const auto& strmap = map->to_string_map();
            for (const auto& atr : strmap)
                node->children.emplace_back(scene_info_for_attribute(atr.first, atr.second.get(), sensor));
        }

        return node;
    }

    assert(false);
    return nullptr;
}

std::unique_ptr<scene_info_t> wt::gui::build_scene_info(
        const std::string_view prefix,
        std::optional<std::string> name,
        const wt::scene::element::info_t& info,
        const sensor::sensor_t& sensor) noexcept {
    auto node = std::make_unique<scene_info_t>();

    if (name) {
        auto data = info.type;
        if (!info.cls.empty())
            data = "(" + info.cls + ") " + data;

        node->name = std::move(*name);
        node->data = std::move(data);
    } else {
        node->name = info.cls;
        node->data = info.type;
    }
    if (!prefix.empty())
        node->data = std::string{ prefix } + " " + node->data;
    node->id = info.id;

    for (const auto& a : info.attribs)
        node->children.emplace_back(scene_info_for_attribute(a.first, a.second.get(), sensor));

    return node;
}

inline void draw_imgui_table_node_graphic(const impl_t* pimpl, scene_info_t& node) noexcept {
    const auto* table = ImGui::GetCurrentTable();

    // span all columns
    const ImRect row_r(table->WorkRect.Min.x, table->BgClipRect.Min.y, table->WorkRect.Max.x, table->RowPosY2);
    ImGui::PushClipRect(table->BgClipRect.Min, table->BgClipRect.Max, false);
    const auto popup_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse;

    if (node.image) {
        const auto image_h = 100.f;
        if (ImGui::ImageButtonEx(ImGui::GetID(&node.buttonid), (ImTextureID)*node.image,
                                 ImVec2{ image_h*node.image->width/node.image->height,image_h },
                                 ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1))) {
            // show popup with image
            node.popup_open = true;
            ImGui::OpenPopupEx(ImGui::GetID(&node.popupid));
        }

        if (node.popup_open) {
            constexpr auto padding = 1;
            // limit window size to keep image aspect ratio
            ImGui::SetNextWindowSize({ 800,600 }, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints({40,40}, ImVec2{(float)m::inf,(float)m::inf},
                [](ImGuiSizeCallbackData* data) {
                    const auto frame = ImVec2{ 
                        ImGui::GetStyle().WindowBorderSize*2, ImGui::GetStyle().WindowBorderSize*2 + ImGui::GetFrameHeight()
                    } - ImVec2{ padding,padding }*2.f;
                    const auto* img = (const gl_image_t*)data->UserData;
                    const auto img_size = ImVec2{ (float)img->width,(float)img->height };
                    const auto cs = data->CurrentSize - frame;
                    const auto ratio = cs.x / img_size.x;
                    data->DesiredSize.y = (img_size * ratio + frame).y;
                }, node.image.get());
            if (imgui_begin_popup_ex(node.popup_lbl, &node.popup_open,
                                     ImGui::GetID(&node.popupid), popup_flags)) {
                ImGui::SetCursorPos({ padding, padding + ImGui::GetFrameHeight() });

                const auto frame = ImVec2{ 
                    ImGui::GetStyle().WindowBorderSize*2, ImGui::GetStyle().WindowBorderSize*2 + ImGui::GetFrameHeight()
                } - ImVec2{ padding,padding }*2.f;
                const auto img_size = ImVec2{ (float)node.image->width,(float)node.image->height };
                const auto ratio = (ImGui::GetCurrentWindow()->SizeFull - frame) / img_size;
                const auto size = img_size * m::max(.0f,m::min(ratio.x,ratio.y));
                ImGui::Image((ImTextureID)*node.image, size);
                ImGui::EndPopup();
            }
        }
    }

    if (node.plot) {
        const auto hist_plot_w = 250.f;
        const auto hist_plot_h = 100.f;
        const auto size = ImVec2{ hist_plot_w, hist_plot_h };

        const auto sp = ImGui::GetCursorPos();

        ImGui::SetCursorPos(sp);
        plot_graph(size, *node.plot,
                   pimpl->mono_font, ImPlotFlags_CanvasOnly);
        if (ImGui::IsItemClicked()) {
            node.popup_open = true;
            ImGui::OpenPopupEx(ImGui::GetID(&node.popupid));
        }

        if (node.popup_open) {
            constexpr auto padding = 1;
            ImGui::SetNextWindowSize(ImVec2{ 800,600 }, ImGuiCond_FirstUseEver);
            if (imgui_begin_popup_ex(node.popup_lbl, &node.popup_open,
                                     ImGui::GetID(&node.popupid), popup_flags)) {
                ImGui::SetCursorPos({ padding, padding + ImGui::GetFrameHeight() });

                plot_graph(ImGui::GetContentRegionAvail() - ImVec2{ padding,padding }*2.f, *node.plot, 
                           pimpl->mono_font, ImPlotFlags_Crosshairs);
                ImGui::EndPopup();
            }
        }
    }

    ImGui::TableNextColumn();
    ImGui::TableNextColumn();

    ImGui::PopClipRect();
}

inline void draw_imgui_table_node_data(const scene_info_t& node) noexcept {
    // populate data cells
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(node.data.c_str());
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(node.id.c_str());
}

void draw_inner_node(scene_info_t* node, const impl_t* pimpl, int node_flags, bool root) noexcept {
    ImGui::PushID(node);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    constexpr int root_extra_flags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen;
    constexpr int inner_node_extra_flags = ImGuiTreeNodeFlags_SpanAllColumns;

    int flags = node_flags;
    if (root)
        flags |= root_extra_flags;

    if (!node->children.empty()) {
        const bool open = ImGui::TreeNodeEx(node->name.c_str(), flags | inner_node_extra_flags);

        draw_imgui_table_node_data(*node);

        if (open) {
            for (const auto& c : node->children)
                draw_inner_node(c.get(), pimpl, node_flags, false);

            // if we have a graphic, draw it a new row that spans all columns
            const bool has_graphics = node->image || node->plot;
            if (has_graphics) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                draw_imgui_table_node_graphic(pimpl, *node);
            }

            ImGui::TreePop();
        }
    } else {
        ImGui::TreeNodeEx(node->name.c_str(), flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);

        draw_imgui_table_node_data(*node);
    }
    ImGui::PopID();
}

void scene_info_t::draw_imgui_table_node(const impl_t* pimpl, int node_flags) noexcept {
    draw_inner_node(this, pimpl, node_flags, true);
}
