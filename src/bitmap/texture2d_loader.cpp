/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <stdexcept>

#include <wt/bitmap/texture2d.hpp>

#include <wt/util/unreachable.hpp>
#include <wt/util/format/enum.hpp>
#include <wt/util/format/utils.hpp>


using namespace wt;
using namespace wt::bitmap;


using block_t = texture2d_storage_t::aligned_block_t;


/**
 * @brief Computes the bytes needed for texture storage.
 */
template <texel T>
inline static auto required_texture_bytes(const vec2u32_t tile_size, const vec2u32_t tiles, const int comps) {
    const auto tile_texels = tile_size.x*tile_size.y;

    const auto b = std::size_t(tiles.x)*tiles.y * tile_texels * sizeof(T) * comps;
    constexpr auto alignment = sizeof(block_t);
    return (b + alignment-1)/alignment * alignment;
}

/**
 * @brief Encodes a bitmap as a texture with tiled memory layout.
 * @param target target to write to; target's allocated byte size must be required_texture_bytes().
 */
template <texel T>
inline void encode_texture(const vec2u32_t tile_size, const bitmap2d_t<T>& level, const vec2u32_t tiles, T* target) {
    const auto comps = level.components();
    const auto tile_texels = tile_size.x*tile_size.y;

    for (auto ty=0u;ty<tiles.y;++ty)
    for (auto tx=0u;tx<tiles.x;++tx) {
        for (auto y=0u;y<tile_size.y;++y)
        for (auto x=0u;x<tile_size.x;++x) {
            const auto bmx = tx*tile_size.x+x, bmy = ty*tile_size.y+y;
            if (bmx<level.width() && bmy<level.height())
                std::copy_n(&level(bmx,bmy,0), comps, target + comps*(x+y*tile_size.x));
        }
        target += tile_texels * comps;
    }
}

/**
 * @brief Constructs a texture storage from a bitmap.
 */
template <texel T>
inline auto texture_storage_from_bitmap(const bitmap2d_t<T>& bitmap) {
    if (bitmap.pixel_layout()==pixel_layout_e::Custom)  // Unsupported
        throw std::runtime_error("(texture2d loader) Bitmaps with custom pixel layout are unsupported");

    texture2d_storage_t::tex_info_t info;
    std::unique_ptr<block_t[]> data;

    const auto ts = texture2d_storage_t::tile_size;

    const auto& rect = bitmap.dimensions();
    const auto tiles = (rect + ts - vec2u32_t{ 1 }) / ts;
    const auto bytes = required_texture_bytes<T>(ts, tiles, bitmap.components());
    assert(bytes%sizeof(block_t) == 0);

    // encode tiled data
    data = std::make_unique<block_t[]>(bytes/sizeof(block_t));
    encode_texture(ts, bitmap, tiles, (T*)data.get());
        
    info = {
        .dim = rect,
        .tiles = tiles,
    };

    return texture2d_storage_t{
        .data = std::move(data),
        .info = info,
        .comp_size = sizeof(T),
        .layout = bitmap.pixel_layout(),
    };
}


namespace detail_texture_loader {

template <typename TexelT, pixel_layout_e layout, texture_filter_type_e texfilter>
inline std::unique_ptr<texture2d_t> texture2d_for_colour_encoding(
        texture2d_storage_t&& storage,
        const texture2d_config_t& config,
        colour_encoding_t ce) {
    switch (ce.type()) {
    case wt::bitmap::colour_encoding_type_e::linear:
        return std::make_unique<texture2d_tmpl_t<TexelT,texfilter,layout,wt::bitmap::colour_encoding_type_e::linear>>(
                std::move(storage), config, ce);
    case wt::bitmap::colour_encoding_type_e::sRGB:
        return std::make_unique<texture2d_tmpl_t<TexelT,texfilter,layout,wt::bitmap::colour_encoding_type_e::sRGB>>(
                std::move(storage), config, ce);
    case wt::bitmap::colour_encoding_type_e::gamma:
        return std::make_unique<texture2d_tmpl_t<TexelT,texfilter,layout,wt::bitmap::colour_encoding_type_e::gamma>>(
                std::move(storage), config, ce);
    }
    
    unreachable();
}
template <typename TexelT, pixel_layout_e layout>
inline std::unique_ptr<texture2d_t> texture2d_for_filter(
        texture2d_storage_t&& storage,
        const texture2d_config_t& config,
        colour_encoding_t ce) {
    switch (config.filter) {
    case texture_filter_type_e::nearest:
        return texture2d_for_colour_encoding<TexelT,layout,texture_filter_type_e::nearest>(
                std::move(storage), config, ce);
    case texture_filter_type_e::bilinear:
        return texture2d_for_colour_encoding<TexelT,layout,texture_filter_type_e::bilinear>(
                std::move(storage), config, ce);
    case texture_filter_type_e::bicubic:
        return texture2d_for_colour_encoding<TexelT,layout,texture_filter_type_e::bicubic>(
                std::move(storage), config, ce);
    }
    
    unreachable();
}
template <typename TexelT>
inline std::unique_ptr<texture2d_t> texture2d_for_layout(
        texture2d_storage_t&& storage,
        const texture2d_config_t& config,
        colour_encoding_t ce) {
    switch (storage.layout.layout) {
    case pixel_layout_e::L:
        return texture2d_for_filter<TexelT,pixel_layout_e::L>(std::move(storage), config, ce);
    case pixel_layout_e::LA:
        return texture2d_for_filter<TexelT,pixel_layout_e::LA>(std::move(storage), config, ce);
    case pixel_layout_e::RGB:
        return texture2d_for_filter<TexelT,pixel_layout_e::RGB>(std::move(storage), config, ce);
    case pixel_layout_e::RGBA:
        return texture2d_for_filter<TexelT,pixel_layout_e::RGBA>(std::move(storage), config, ce);
    default:
        throw std::runtime_error("(texture2d loader) unrecognized layout");
    }
}

template <typename TexelT>
inline std::unique_ptr<texture2d_t> texture_from_bitmap(
        const wt_context_t& ctx,
        const std::string& name,
        const bitmap2d_load_ret_t<TexelT>& bitmap,
        const texture2d_config_t config,
        std::optional<colour_encoding_t> colour_encoding) {
    auto storage = texture_storage_from_bitmap(bitmap.bitmap);

    wt::logger::cout(verbosity_e::info)
        << "(texture2d loader) Created 2D texture \"" << name << "\" from bitmap." << '\n';

    if (colour_encoding) {
        // warn about change of colour encoding
        if (bitmap.colour_encoding != *colour_encoding)
            wt::logger::cwarn(verbosity_e::info)
                << "(texture2d loader) 2D texture \"" << name << "\": bitmap uses with '"
                << std::format("{}", bitmap.colour_encoding.type()) << "' colour encoding, but desired colour encoding is '"
                << std::format("{}", colour_encoding->type()) << "'. Desired encoding will be used."
                << '\n';
    } else {
        colour_encoding = bitmap.colour_encoding;
    }

    return texture2d_for_layout<TexelT>(std::move(storage), config, *colour_encoding);
}

}

std::unique_ptr<texture2d_t> wt::bitmap::texture2d_t::load_from_path(
        const wt_context_t& ctx,
        const std::filesystem::path& path,
        const texture2d_config_t config,
        std::optional<colour_encoding_t> colour_encoding) {
    // use path relative to scene data as key
    const auto abspath = std::filesystem::absolute(path);
    const auto scene_rel_path = std::filesystem::relative(abspath, ctx.scene_data_path);
    assert(!scene_rel_path.string().empty());
    const auto& name = scene_rel_path.string();
    
    // texel type is decided by path extension
    // EXR - f_t
    if (format::tolower(path.extension().string())==".exr") {
        return detail_texture_loader::texture_from_bitmap<f_t>(
                    ctx,
                    name,
                    load_bitmap2d_exr(path),
                    config, colour_encoding);
    }
    // PNG - unorm8/16
    if (format::tolower(path.extension().string())==".png") {
        const auto bitdepth = load_bitmap2d_png_query_bitdepth(path);
        if (bitdepth==8)
            return detail_texture_loader::texture_from_bitmap<std::uint8_t>(
                        ctx,
                        name,
                        load_bitmap2d_png8(path),
                        config, colour_encoding);
        if (bitdepth==16)
            return detail_texture_loader::texture_from_bitmap<std::uint16_t>(
                        ctx,
                        name,
                        load_bitmap2d_png16(path),
                        config, colour_encoding);
        assert(false);
    }

    throw std::runtime_error("(texture2d loader) Unsupported file type");
}
