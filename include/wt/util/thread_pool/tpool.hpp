/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <vector>

#include <coroutine>
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>

#include <variant>
#include <concepts>

#include <tbb/concurrent_queue.h>

#include <wt/util/unique_function.hpp>
#include <wt/util/thread_pool/tpool_worker_arena.hpp>

namespace wt::thread_pool {

inline const std::size_t native_concurrency = std::thread::hardware_concurrency();

/**
 * @brief Simple static thread pool.
 *        Optimized for the case of a few consumers that produce a lot of work, like a renderer.
 * 
 */
class tpool_t {
private:
    struct alignas(64) task_t {
        std::variant<
            unique_function<void()>,
            std::coroutine_handle<>
        > f;
    };
    struct mutable_data_t {
        alignas(64) tbb::concurrent_queue<task_t> task_queue;

        alignas(64) std::mutex m;
        std::size_t enqueued_tasks = 0;
        bool terminate_flag = false;
        alignas(64) std::condition_variable cv;
    };

    std::vector<std::thread> spawn_threads(std::size_t threads);

private:
    mutable mutable_data_t d;
    std::vector<std::thread> threads;

private:
    void on_enqueue() const noexcept {
        {
            std::unique_lock l(d.m);
            ++d.enqueued_tasks;
        }
        d.cv.notify_one();
    }

    void enqueue_coro(std::coroutine_handle<> coro) const noexcept {
        d.task_queue.emplace(task_t{ .f = coro });
        on_enqueue();
    }

public:
    /**
     * @brief Construct a new tpool_t with `threads' count of threads.
     */
    tpool_t(std::size_t threads = native_concurrency)
        : threads(spawn_threads(threads))
    {}
    ~tpool_t() {
        {
            // raise terminate flag
            std::unique_lock l(d.m);
            d.terminate_flag = true;
        }

        // notify and wait upon all
        d.cv.notify_all();
        for (auto& t : threads)
            t.join();
    }

    /**
     * @brief Get thread count in the thread pool.
     */
    [[nodiscard]] inline auto thread_count() const noexcept { return threads.size(); }

    /**
     * @brief Returns the threads in the pool.
     */
    [[nodiscard]] inline const auto& get_threads() const noexcept { return threads; }

    /**
     * @brief Enqueues a task and returns a future.
     */
    template <std::invocable F>
    [[nodiscard]] inline auto enqueue(F&& f) const noexcept {
        using R = std::invoke_result_t<F&&>;

        std::packaged_task<R()> pt{ std::forward<F>(f) };
        auto future = pt.get_future();
        d.task_queue.emplace(task_t{ .f = std::move(pt) });

        on_enqueue();

        return future;
    }

    inline auto coro() const noexcept {
        struct coro_awaiter_t {
            const tpool_t* tp;

            [[nodiscard]] constexpr bool await_ready() const noexcept { return false; }
            constexpr void await_resume() const noexcept {}
            void await_suspend(std::coroutine_handle<> coro) const noexcept {
                tp->enqueue_coro(coro);
            }
        };
        return coro_awaiter_t{ this };
    }

    /**
     * @brief Create a worker arena, each worker arena is default initialized T().
     *        Each worker thread in this thread pool can access its local arena via tpool_worker_arena_t::get().
     */
    template <std::default_initializable T>
    [[nodiscard]] inline auto create_worker_arena() const noexcept {
        return tpool_worker_arena_t<T>{ thread_count() };
    }
    /**
     * @brief Create a worker arena, each worker arena is copy constructed from ``arena''.
     *        Each worker thread in this thread pool can access its local arena via tpool_worker_arena_t::get().
     */
    template <std::copy_constructible T>
    [[nodiscard]] inline auto create_worker_arena(const T& arena) const noexcept {
        return tpool_worker_arena_t<T>{ thread_count(), arena };
    }
};

}
