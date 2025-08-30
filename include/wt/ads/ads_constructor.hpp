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
#include <chrono>
#include <wt/ads/ads.hpp>

namespace wt::ads::construction {

/**
 * @brief Generic interface for accelerating data structure (ADS) construction.
 */
class ads_constructor_t {
protected:
    std::chrono::high_resolution_clock::duration build_time;

public:
    virtual std::unique_ptr<ads_t> get() && = 0;
    virtual ~ads_constructor_t() noexcept = default;

    [[nodiscard]] virtual std::shared_ptr<std::string> get_state_description() const noexcept { return nullptr; }

    [[nodiscard]] inline auto get_construction_time() const noexcept { return build_time; }
};

}  // namespace wt::ads
