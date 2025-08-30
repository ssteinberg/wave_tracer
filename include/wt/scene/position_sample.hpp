/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <optional>

#include <wt/interaction/intersection.hpp>
#include <wt/sampler/density.hpp>

#include <wt/math/common.hpp>

namespace wt {

struct position_sample_t {
    pqvec3_t p;

    area_sampling_pd_t ppd;

    std::optional<intersection_surface_t> surface;
};

}
