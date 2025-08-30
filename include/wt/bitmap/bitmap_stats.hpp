/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#pragma once

#include <chrono>
#include <wt/util/statistics_collector/stat_collector_registry.hpp>
#include <wt/util/statistics_collector/stat_histogram.hpp>
#include <wt/util/statistics_collector/stat_timings.hpp>

namespace wt::bitmap::bitmap_stats {

#if defined(_additional_bitmap_stats)
static constexpr auto additional_bitmap_counters = true;
#else
static constexpr auto additional_bitmap_counters = false;
#endif

namespace detail {

using namespace wt::stats;

struct bitmap_stat_counters_t {
    stat_timings_t* bmp_filter_timings = additional_bitmap_counters ?
        stat_collector_registry_t::instance().make_collector<stat_timings_t>("(BMP) timings tex filter") :
        nullptr;

    stat_histogram_t<127>* bmp_texels_per_filter = additional_bitmap_counters ?
        stat_collector_registry_t::instance().make_collector<stat_histogram_t<127>>("(BMP) texels per query", 1) :
        nullptr;
};

}

inline thread_local detail::bitmap_stat_counters_t bitmap_stats_counters{};

inline void on_bitmap_filter(int texels, std::chrono::high_resolution_clock::time_point start) noexcept {
    if constexpr (additional_bitmap_counters) {
        const auto d = std::chrono::high_resolution_clock::now() - start;
        bitmap_stats_counters.bmp_filter_timings->record(d);
        bitmap_stats_counters.bmp_texels_per_filter->increment_count_of(texels);
    }
}

}  // namespace wt::bitmap_stats
