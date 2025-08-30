/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/common.hpp>
#include <wt/bitmap/pixel_layout.hpp>

namespace wt::sensor {

/**
 * @brief Opaque block handle for a sensor.
 */
struct block_handle_t {
    void* block = nullptr;

    // block position in film (position on film that maps to (0,0) of block, ignoring padding)
    vec3u32_t position;
    // block size
    vec3u32_t size;

    block_handle_t() = default;
    block_handle_t(block_handle_t&&) = default;

    block_handle_t(const block_handle_t&) = delete;
    block_handle_t& operator=(const block_handle_t&) = delete;
};

}
