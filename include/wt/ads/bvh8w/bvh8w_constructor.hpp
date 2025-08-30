/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#pragma once

#include <memory>

#include <vector>
#include <optional>

#include <wt/scene/shape.hpp>
#include <wt/wt_context.hpp>

#include <wt/ads/util.hpp>
#include <wt/ads/ads_constructor.hpp>
#include "bvh8w.hpp"

namespace wt::ads::construction {

/**
 * @brief Constructs an 8-wide SAH BVH.
 */
class bvh8w_constructor_t final : public ads_constructor_t {
public:
    bvh8w_constructor_t(std::vector<std::shared_ptr<shape_t>> objs,
                        const wt::wt_context_t &context,
                        std::optional<progress_callback_t> progress_callbacks = {});

    std::unique_ptr<ads_t> get() && override {
        assert(bvh8w);
        return std::move(bvh8w);
    }

private:
    std::unique_ptr<bvh8w_t> bvh8w;
    progress_track_t pt;
};

}
