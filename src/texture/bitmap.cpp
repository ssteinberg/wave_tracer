/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <optional>
#include <filesystem>
#include <string>

#include <wt/scene/loader/node.hpp>
#include <wt/scene/element/attributes.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/wt_context.hpp>

#include <wt/bitmap/texture2d.hpp>
#include <wt/texture/bitmap.hpp>

#include <wt/spectrum/rgb.hpp>
#include <wt/spectrum/uniform.hpp>

#include <wt/math/common.hpp>

using namespace wt;
using namespace texture;


scene::element::info_t bitmap_t::description() const {
    using namespace scene::element;

    auto tex_info = tex->description();
    auto tex_desc = std::move(tex_info.attribs);
    tex_desc.emplace("cls", attributes::make_string(tex_info.cls));

    return info_for_scene_element(*this, "bitmap", {
        { "texture", attributes::make_map(std::move(tex_desc)) },
    });
}


[[nodiscard]] inline std::shared_ptr<spectrum::spectrum_real_t> mean_spectrum_for_texture(
        std::string id,
        const bitmap_t::texture2d_t& tex) noexcept {
    const auto mv = tex.mean_value();
    if (tex.pixel_layout() == bitmap::pixel_layout_e::RGB ||
        tex.pixel_layout() == bitmap::pixel_layout_e::RGBA) {
        // RGB spectrum
        return std::make_shared<spectrum::rgb_t>(std::move(id), vec3_t{ mv });
    } else {
        // uniform spectrum
        return std::make_shared<spectrum::uniform_t>(std::move(id), mv.x);
    }
}

std::shared_ptr<bitmap_t> bitmap_t::load(std::string id, 
                                         scene::loader::loader_t* loader, 
                                         const scene::loader::node_t& node, 
                                         const wt::wt_context_t &context) {
    std::optional<std::filesystem::path> filename;
    std::optional<f_t> gamma;
    std::optional<bitmap::texture_filter_type_e> texture_filter;
    std::optional<bitmap::texel_clamp_mode_e> texel_clamp_mode;
    std::optional<bitmap::texture_wrap_mode_e> wrap_mode, uwrap_mode, vwrap_mode;
    std::optional<bitmap::colour_encoding_type_e> colour_encoding_type;

    // filename can also be given inline as bitmap="<filename>", used as a shorthand.
    auto shorthand_filename = node["bitmap"];
    const bool has_shorthand_filename = shorthand_filename.length()>0;
    if (has_shorthand_filename)
        filename = std::move(shorthand_filename);

    for (auto& item : node.children_view()) {
    try {
        if (!has_shorthand_filename &&
            scene::loader::read_attribute(item, filename))
            continue;
        if (!scene::loader::read_attribute(item, "gamma", gamma) &&
            !scene::loader::read_enum_attribute(item, "filter_type", texture_filter) &&
            !scene::loader::read_enum_attribute(item, "texel_clamp_mode", texel_clamp_mode) &&
            !scene::loader::read_enum_attribute(item, "wrap_mode", wrap_mode) &&
            !scene::loader::read_enum_attribute(item, "wrap_mode_u", uwrap_mode) &&
            !scene::loader::read_enum_attribute(item, "wrap_mode_v", vwrap_mode) &&
            !scene::loader::read_enum_attribute(item, "colour_encoding", colour_encoding_type))
            logger::cwarn()
                << loader->node_description(item)
                << "(bitmap texture loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(bitmap texture loader) " + std::string{ exp.what() }, item);
    }
    }

    // parse config
    bitmap::texture2d_config_t cfg;
    
    if (texture_filter)
        cfg.filter = *texture_filter;

    if (texel_clamp_mode)
        cfg.texel_clamp_mode = *texel_clamp_mode;
    
    if (wrap_mode) {
        if (uwrap_mode || vwrap_mode)
            throw scene_loading_exception_t("(bitmap texture loader) 'wrap_mode' cannot be specified together with 'wrap_mode_u' or 'wrap_mode_v'", node);
        cfg.uwrap = cfg.vwrap = *wrap_mode;
    }
    if (uwrap_mode)
        cfg.uwrap = *uwrap_mode;
    if (vwrap_mode)
        cfg.vwrap = *vwrap_mode;

    std::optional<bitmap::colour_encoding_t> colour_encoding;
    if (colour_encoding_type)
        colour_encoding = *colour_encoding_type;
    if (gamma) {
        if (colour_encoding && colour_encoding->type()!=bitmap::colour_encoding_type_e::gamma)
            throw scene_loading_exception_t("(bitmap texture loader) Conflicting colour encoding", node);
        if (*gamma<=0)
            throw scene_loading_exception_t("(bitmap texture loader) 'gamma' must be positive", node);
        colour_encoding = bitmap::colour_encoding_t{ bitmap::colour_encoding_type_e::gamma, *gamma };
    }

    // create bitmap texture
    auto bmp = std::make_shared<bitmap_t>(id, bitmap_t::private_token_t{});

    if (!filename)
        throw scene_loading_exception_t("(bitmap texture loader) path must be provided", node);
    const auto path = context.resolve_path(std::filesystem::path{ *filename });
    if (!path)
        throw scene_loading_exception_t("(bitmap texture loader) bitmap '" + filename->string() + "' not found.", node);

    // load texture in background
    std::promise<void> promise;
    bmp->defered_load_future = promise.get_future();
    loader->enqueue_loading_task(bmp.get(), bmp->aux_task_name(), 
        [bmp, sid=std::move(id), &context, path=*path, cfg, colour_encoding, promise=std::move(promise)]() mutable {
        // load bitmap
        bmp->tex = bitmap::texture2d_t::load_from_path(context, path, cfg, colour_encoding);
        // compute mean spectrum
        bmp->avg_spectrum = mean_spectrum_for_texture(std::move(sid), *bmp->tex);

        promise.set_value();
    });

    return bmp;
}
