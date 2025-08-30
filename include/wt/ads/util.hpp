/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <functional>

#include <wt/util/unique_function.hpp>
#include <wt/math/common.hpp>

namespace wt::ads::construction {

/**
 * @brief Callbacks that will be called with ADS construction updates.
 */
struct progress_callback_t {
    /** @brief Progress update callback.
     */
    unique_function<void(f_t) const noexcept> progress_update;
    /** @brief Called on successful completion.
     */
    unique_function<void() const noexcept> on_finish;
    /** @brief Provides a description of latest construction status.
     */
    unique_function<void(std::string) const noexcept> status_description_update;
};

struct progress_track_t {
    float proportion = 1.0f, start;
    std::chrono::milliseconds last_update{0};

    std::optional<progress_callback_t> callbacks;

    inline progress_track_t(const f_t start=0) noexcept : start(start)
    {}

    inline void set_status(std::string status) noexcept {
        if (callbacks && callbacks->status_description_update)
            callbacks->status_description_update(std::move(status));
    }
    inline void set_progress(f_t p) noexcept {
        if (callbacks && callbacks->progress_update) {
            callbacks->progress_update(
                (start + (1-start) * m::clamp01(p) * proportion));
        }
    }
    inline void complete() const noexcept {
        if (callbacks && callbacks->on_finish)
            callbacks->on_finish();
    }
};

}
