/*
*
* Wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <map>
#include <string>
#include <filesystem>

#include <wt/bitmap/bitmap.hpp>
#include <wt/bitmap/common.hpp>

namespace wt::bitmap {

// TODO: colour spaces and colour encoding in OpenEXR and libpng.

/**
 * @brief Writes a floating-point OpenEXR bitmap.
 * @param channel_names (optional) if provided, writes the names for each channel
 * @param attributes (optional) map of arbitrary attributes to write in the EXR
 */
void write_bitmap2d_exr(const std::filesystem::path &path,
                        const bitmap2d_t<float> &bitmap,
                        const std::vector<std::string>& channel_names = {},
                        const std::map<std::string,std::string>& attributes = {});

/**
 * @brief Writes a floating-point OpenEXR bitmap. Converts the input `bitmap` to a floating-point bitmap first.
 * @param channel_names (optional) if provided, writes the names for each channel
 * @param attributes (optional) map of arbitrary attributes to write in the EXR
 */
template <texel T>
inline void write_bitmap2d_exr(const std::filesystem::path &path,
                               const bitmap2d_t<T> &bitmap,
                               const std::vector<std::string>& channel_names = {},
                               const std::map<std::string,std::string>& attributes = {}) {
    write_bitmap2d_exr(
        path, 
        bitmap.template convert_texels<float>(),
        channel_names,
        attributes
    );
}

/**
 * @brief Writes a unorm8 PNG bitmap. Assume sRGB colour encoding.
 */
void write_bitmap2d_png(const std::filesystem::path &path,
                        const bitmap2d_t<std::uint8_t> &bitmap);
/**
 * @brief Writes a unorm16 PNG bitmap. Assumes linear colour encoding.
 */
void write_bitmap2d_png(const std::filesystem::path &path,
                        const bitmap2d_t<std::uint16_t> &bitmap);

}
