/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cassert>
#include <vector>

#include <wt/util/thread_pool/utils.hpp>

namespace wt::thread_pool {

/**
 * @brief A container of thread-local resources.
 *        Creates an object T() for each thread in the supplied thread pool.
 *        Thread pool threads then may access their own local resource using get(), while any readers may access all the resources at all times.
 *
 *        tpool_worker_arena_t is created from a tpool_t.
 */
template <typename T>
class tpool_worker_arena_t {
    friend class tpool_t;

private:
    using arenas_t = std::vector<T>;    
    arenas_t arenas;

private:
    inline tpool_worker_arena_t(std::size_t count) noexcept : arenas(count) {}
    inline tpool_worker_arena_t(std::size_t count, const T& t) noexcept : arenas(count, t) {}

public:
    tpool_worker_arena_t() = delete;

    /**
     * @brief Must only be called from a thread pool worker.
     */
    [[nodiscard]] inline auto& get() noexcept {
        assert(is_this_thread_tpool_worker());

        const auto tid = tpool_worker_tid();
        return arenas[tid];
    }
    /**
     * @brief Must only be called from a thread pool worker.
     */
    [[nodiscard]] inline const auto& get() const noexcept {
        assert(is_this_thread_tpool_worker());

        const auto tid = tpool_worker_tid();
        return arenas[tid];
    }

    [[nodiscard]] inline const auto& operator[](std::size_t idx) const noexcept { return arenas[idx]; }

    [[nodiscard]] inline auto size() const noexcept { return arenas.size(); }

    [[nodiscard]] inline auto begin() const noexcept { return arenas.begin(); }
    [[nodiscard]] inline auto end() const noexcept { return arenas.end(); }
    [[nodiscard]] inline auto rbegin() const noexcept { return arenas.rbegin(); }
    [[nodiscard]] inline auto rend() const noexcept { return arenas.rend(); }
    [[nodiscard]] inline auto cbegin() const noexcept { return arenas.cbegin(); }
    [[nodiscard]] inline auto cend() const noexcept { return arenas.cend(); }
    [[nodiscard]] inline auto crbegin() const noexcept { return arenas.crbegin(); }
    [[nodiscard]] inline auto crend() const noexcept { return arenas.crend(); }
};

}
