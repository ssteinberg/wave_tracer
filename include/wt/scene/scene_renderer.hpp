/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <future>
#include <vector>
#include <queue>

#include <chrono>
#include <atomic>

#include <wt/wt_context.hpp>
#include <wt/util/preview/preview_interface.hpp>

#include <wt/integrator/integrator.hpp>
#include <wt/ads/ads.hpp>

#include <wt/util/unique_function.hpp>
#include <wt/math/common.hpp>

#include "scene.hpp"
#include "render_results.hpp"
#include "interrupts.hpp"

namespace wt {

namespace scene {

using time_point_t = std::chrono::steady_clock::time_point;
using duration_t = std::chrono::steady_clock::duration;

/**
 * @brief Scene renderer options.
 */
struct render_opts_t {
    /**
     * @brief Callbacks that will be called with rendering progress updates.
     */
    struct progress_callback_t {
        /** @brief Progress update callback.
         *         First argument is sensor id; second argument is progress.
         */
        unique_function<void(const std::string&, const f_t) const noexcept> progress_update;
        /** @brief Rendering complete callback.
         *         First argument is sensor id; second argument is total elapsed time.
         */
        unique_function<void(const std::string&, const duration_t&) const noexcept> on_complete;
        /** @brief Rendering terminated callback. First argument is sensor id.
         */
        unique_function<void(const std::string&) const noexcept> on_terminate;
    };

    /** @brief Progress callback handlers, if any.
     */
    std::optional<progress_callback_t> progress_callback;
    /** @brief Preview interface to use, if any.
     */
    const preview_interface_t* previewer = nullptr;
};

enum class rendering_state_t : std::uint8_t {
    completed_successfully,
    terminated,
    rendering,
    pausing,
    paused,
};

/**
 * @brief Describes the rendering state.
 */
struct rendering_status_t {
    time_point_t start_time;
    duration_t elapsed_rendering_time;
    rendering_state_t state;

    std::size_t total_blocks, completed_blocks, blocks_in_progress;

    [[nodiscard]] inline f_t progress() const noexcept {
        return total_blocks>0 ? completed_blocks / (f_t)total_blocks : 0;
    }
    [[nodiscard]] const auto estimated_remaining_rendering_time() const noexcept {
        const auto p = progress();
        const auto eta = std::chrono::nanoseconds(p > 0 ?
                std::int64_t(m::ceil(float(elapsed_rendering_time.count()) / p)) : 0l);
        return eta;
    }
};

}

/**
 * @brief Scene renderer, handles rendering loop.
 */
class scene_renderer_t {
public:
    using interrupt_t = std::unique_ptr<scene::interrupts::interrupt_t>;

private:
    // multiplicative factor of desired parallelism for enqueueing parallel jobs
    static constexpr f_t parallel_jobs_factor = 1.5;

    std::future<scene::render_result_t> future;

    std::atomic_flag interrupt_flag;
    mutable std::unique_ptr<std::mutex> interrupts_queue_mutex = std::make_unique<std::mutex>();
    mutable std::queue<interrupt_t> interrupts_queue;

    struct alignas(64) renderer_state_t {
        bool paused = false, saved_paused_state;
        bool terminated = false, completed = false;

        std::size_t total_jobs, jobs_enqueued = 0, jobs_completed = 0;

        scene::time_point_t start_time{}, last_checkpoint{};
        scene::duration_t elpased_time_till_last_checkpoint{};

        std::vector<interrupt_t> pending_capture_intermediate_interrupts;

        [[nodiscard]] inline bool has_pending_capture_interrupts() const noexcept {
            return !pending_capture_intermediate_interrupts.empty();
        }
        [[nodiscard]] inline bool has_pending_interrupts() const noexcept {
            return has_pending_capture_interrupts();
        }

        [[nodiscard]] inline auto elapsed_time() const noexcept {
            auto ret = elpased_time_till_last_checkpoint;
            if (!paused && !completed && !terminated && start_time>scene::time_point_t{})
                ret += std::chrono::steady_clock::now() - last_checkpoint;
            return ret;
        }

        void checkpoint(bool paused = false) noexcept {
            const auto& now = std::chrono::steady_clock::now();
            if (!paused)
                elpased_time_till_last_checkpoint += now - last_checkpoint;
            last_checkpoint = now;
        }

        void process_pending_interrupts(void*) noexcept;
    } state;

private:
    void process_interrupts(void*);

    scene::render_result_t render(
            const scene_t* scene,
            const wt_context_t& ctx, const ads::ads_t& ads,
            const scene::render_opts_t render_opts);

public:
    /**
     * @brief Queues rendering.
     *        Rendering is launched based on the launch policy ``launch_policy``: asynchronously or deferred.
     *        With deferred policy, rendering starts only when get() is called.
     */
    scene_renderer_t(const scene_t& scene,
                     const wt_context_t& ctx, const ads::ads_t& ads,
                     std::launch launch_mode = std::launch::async,
                     scene::render_opts_t render_opts = {});

    /**
     * @brief Retrieves rendering results. This is a blocking operation.
     */
    [[nodiscard]] auto get() { return future.get(); }

    /**
     * @brief Waits for rendering to complete. This is a blocking operation.
     */
    void wait() { future.wait(); }
    /**
     * @brief Waits for rendering to complete. This is a blocking operation.
     *        When ``wait_duration`` is 0, no blocking occurs and rendering state is returned.
     * @param wait_duration maximal time to wait.
     * @return std::future_status
     */
    template<typename Rep, typename Period>
    [[nodiscard]] auto wait_for(const std::chrono::duration<Rep, Period>& wait_duration) {
        return future.wait_for(wait_duration);
    }

    /**
     * @brief Queues an interrupt. See `wt::scene::interrupts`.
     *        (Thread safe).
     */
    void interrupt(interrupt_t intr) {
        {
            std::unique_lock l(*interrupts_queue_mutex);
            interrupts_queue.emplace(std::move(intr));
        }
        interrupt_flag.test_and_set(std::memory_order_release);
    }

    /**
     * @brief Queries the rendering status. Thread safe, returned results might be stale.
     */
    [[nodiscard]] scene::rendering_status_t rendering_status() const noexcept;
};

}
