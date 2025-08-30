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

namespace wt {

/**
 * @brief Measure of a sample or distribution query.
 */
enum class measure_e : std::int8_t {
    discrete,
    continuos,
};

}
