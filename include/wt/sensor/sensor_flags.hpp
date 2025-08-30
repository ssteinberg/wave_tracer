/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cstdint>

namespace wt::sensor {

enum sensor_write_flags_e : std::uint8_t {
    /**
     * @brief Integrator splats to blocks, e.g., via sensor pixel samples.
     */
    writes_block_splats=1<<0,

    /**
     * @brief Integrator splats to arbitrary sensor elements, e.g., via direct sensor samples.
     */
    writes_direct_splats=1<<1,
};

}
