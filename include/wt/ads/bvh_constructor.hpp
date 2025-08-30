/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 * Authors:  Umut Emre, Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#pragma once

#include <memory>
#include <vector>
#include <wt/scene/shape.hpp>
#include <wt/wt_context.hpp>

#include "bvh.hpp"
#include <wt/ads/util.hpp>

namespace wt::ads::construction {

/**
 * @brief Constructs an SAH BVH.
 */
class bvh_constructor_t {
public:
    bvh_constructor_t(std::vector<std::shared_ptr<shape_t>> objs,
                      const wt::wt_context_t &context,
                      progress_track_t& pt);

    std::unique_ptr<bvh_t> bvh;

};

}  // namespace wt::ads
