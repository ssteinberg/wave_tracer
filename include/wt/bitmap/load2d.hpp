/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <filesystem>

#include <wt/math/common.hpp>
#include <wt/bitmap/bitmap.hpp>
#include <wt/bitmap/common.hpp>

namespace wt::bitmap {

/**
 * @brief Helper structure that holds return value from bitmap loaders.
 */
template <typename T>
struct bitmap2d_load_ret_t {
    bitmap2d_t<T> bitmap;

    /** @brief The colour encoding of the loaded bitmap, as reported by the loader. */
    colour_encoding_t colour_encoding;
};


/**
 * @brief Loads a floating-point OpenEXR bitmap. Throws on errors.
 */
bitmap2d_load_ret_t<f_t> load_bitmap2d_exr(const std::filesystem::path &path);


/**
 * @brief Queries the bit depth of a PNG bitmap. Throws on errors.
 */
std::size_t load_bitmap2d_png_query_bitdepth(const std::filesystem::path &file_name);

/**
 * @brief Loads a unorm8 PNG bitmap. Throws on errors.
 */
bitmap2d_load_ret_t<std::uint8_t> load_bitmap2d_png8(const std::filesystem::path &file_name);
/**
 * @brief Loads a unorm16 PNG bitmap. Throws on errors.
 */
bitmap2d_load_ret_t<std::uint16_t> load_bitmap2d_png16(const std::filesystem::path &file_name);

}
