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

namespace wt::thread_pool {

class tpool_tids_t {
    friend class tpool_t;

private:
    static thread_local std::uint32_t tid;

public:
    static constexpr std::uint32_t default_tid = -1;
    static auto get_tid() noexcept { return tid; }
};

/**
 * @brief Returns TRUE if this thread is a thread pool worker.
 */
[[nodiscard]] inline bool is_this_thread_tpool_worker() noexcept {
    return tpool_tids_t::get_tid() != tpool_tids_t::default_tid;
}
/**
 * @brief Returns the index of this thread pool worker, guaranteed to be between 0 and tpool_t::thread_count()-1.
 *        Returns tpool_tids_t::default_tid if this thread is not a thread pool worker.
 */
[[nodiscard]] inline auto tpool_worker_tid() noexcept {
    return tpool_tids_t::get_tid();
}

}
