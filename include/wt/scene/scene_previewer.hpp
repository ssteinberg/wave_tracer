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
#include <map>
#include <chrono>
#include <thread>
#include <atomic>

#include <wt/scene/scene_renderer.hpp>

#include <wt/math/common.hpp>
#include <wt/bitmap/bitmap.hpp>

#include <wt/util/preview/preview_interface.hpp>

#include <wt/sensor/film/film_storage.hpp>
#include <wt/sensor/response/tonemap/tonemap.hpp>

namespace wt::scene {

class scene_previewer_t {
private:
    struct alignas(64) sensor_t {
        const sensor::film_storage_handle_t* film_handle = nullptr;

        mutable std::atomic_flag preview_processed = true;
        mutable f_t fractional_spe_complete = 0;
    };
    struct alignas(64) termination_signal_t {
        std::condition_variable signal;
        bool flag = false;

        std::size_t padding = 0;
        std::mutex m;
    };

    const preview_interface_t* previewer;
    std::map<std::string, std::unique_ptr<sensor_t>> sensors;

    std::thread preview_thread;
    mutable termination_signal_t terminate_flag;

public:
    using sensors_map_t = std::map<std::string, const sensor::film_storage_handle_t*>;

private:
    void preview(const std::string& name, const sensor_t& s) const noexcept {
        // TODO: preview for non-2d sensors
        if (s.film_handle->dimensions_count()!=2) return;

        const auto spe = (std::size_t)(m::round(s.fractional_spe_complete) + f_t(.5));
        if (s.film_handle->is_polarimetric() && previewer->polarimetric_preview()) {
            previewer->update(name,
                              s.film_handle->develop_lin_stokes_d2(spe),
                              s.fractional_spe_complete,
                              s.film_handle->get_tonemap());
        } else {
            previewer->update(name,
                              s.film_handle->develop_lin_d2(spe),
                              s.fractional_spe_complete,
                              s.film_handle->get_tonemap());
        }
    }

    bool process_previews() const noexcept {
        bool updated = false;
        for (const auto& sensor : sensors) {
            if (sensor.second->preview_processed.test_and_set(std::memory_order_acquire))
                continue;
            preview(sensor.first, *sensor.second);
            updated = true;
        }

        return updated;
    }

    void runner() const noexcept {
        using namespace std::chrono; 

        auto last_update_duration = steady_clock::duration{ 0 };

        // process any initial preview requests
        process_previews();

        auto last_update_ts = steady_clock::now();

        for (;!terminate_flag.flag;) {
            const auto preview_delay = previewer->preview_update_interval();
            const auto preview_rate_limit = previewer->preview_update_rate_limit_factor();

            {
                // sleep
                std::unique_lock l(terminate_flag.m);
                terminate_flag.signal.wait_for(l, preview_delay);
            }

            const auto start = steady_clock::now();
            const auto d = start-last_update_ts;
            // ratelimit: sleep for at least `preview_rate_limit' times the duration it took to update last time
            // this avoids excessively using up resources for previews, especially when films are large.
            if (d>=preview_delay &&
                d>=preview_rate_limit*last_update_duration &&
                previewer->available()) {
                // do previews
                const bool updated = process_previews();
                if (updated) {
                    last_update_ts = start;
                    last_update_duration = steady_clock::now() - start;
                }
            }
        }

        // wrap up any outstanding preview requests
        process_previews();
    }

    void terminate() noexcept {
        {
            std::unique_lock l(terminate_flag.m);
            terminate_flag.flag = true;
        }
        terminate_flag.signal.notify_all();
    }

public:
    scene_previewer_t(const render_opts_t& opts,
                      const sensors_map_t& sensors)
        : previewer(opts.previewer)
    {
        for (const auto& s : sensors)
            this->sensors.emplace(s.first, std::make_unique<sensor_t>(s.second));

        preview_thread = std::thread([this]() { this->runner(); });
    }
    ~scene_previewer_t() {
        terminate();
        preview_thread.join();
    }

    void preview(const std::string& sensor_name,
                 const f_t fractional_spe_complete) const noexcept {
        const auto it = sensors.find(sensor_name);
        if (it==sensors.end()) {
            assert(false);
            return;
        }

        const auto& s = *it->second;
        s.fractional_spe_complete = fractional_spe_complete;
        s.preview_processed.clear(std::memory_order_release);
    }
};

}
