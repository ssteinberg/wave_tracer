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
#include <wt/util/statistics_collector/stat_counter_event.hpp>
#include <wt/util/statistics_collector/stat_histogram.hpp>
#include <wt/util/statistics_collector/stat_timings.hpp>
#include <wt/util/statistics_collector/stat_stats.hpp>

namespace wt::integrator::stats {

#if defined(_additional_plt_stats)
static constexpr auto additional_plt_counters = true;
#else
static constexpr auto additional_plt_counters = false;
#endif

namespace counters {

using namespace wt::stats;

inline thread_local auto* plt_fsd_timings = additional_plt_counters ?
    stat_collector_registry_t::instance().make_collector<stat_timings_t>(
        "(PLT) FSD",
        stat_collector_flags_t{ .print_throughput=false }
    ) :
    nullptr;

inline thread_local auto* interaction_event_counter = 
    stat_collector_registry_t::instance().make_collector<stat_counter_event_t<4>>(
        "(PLT) interactions",
        std::array<std::string,4>{ "null", "FSD", "surface", "medium" }
    );

inline thread_local auto* connected_path_depth = 
    stat_collector_registry_t::instance().make_collector<stat_histogram_t<127>>("(PLT) path depths", 1);

inline thread_local auto* interaction_region_size = additional_plt_counters ?
    stat_collector_registry_t::instance().make_collector<stat_stats_t<quantity<isq::area[pow<2>(u::m)]>>>(
        "(PLT) region cross section"
    ) :
    nullptr;

}

inline void record_fsd() noexcept {
}

inline void record_null_interaction() noexcept {
    counters::interaction_event_counter->record(0);
}
inline void record_fsd_interaction(std::chrono::high_resolution_clock::time_point start) noexcept {
    if constexpr (additional_plt_counters) {
        const auto d = std::chrono::high_resolution_clock::now() - start;
        counters::plt_fsd_timings->record(d);
    }
    counters::interaction_event_counter->record(1);
}
inline void record_surface_interaction() noexcept {
    counters::interaction_event_counter->record(2);
}
inline void record_volumetric_interaction() noexcept {
    counters::interaction_event_counter->record(3);
}

inline void record_connected_path(std::size_t depth) noexcept {
    counters::connected_path_depth->increment_count_of(depth);
}

inline void record_interaction_region(const Area auto& region_size) noexcept {
    if constexpr (additional_plt_counters)
        counters::interaction_region_size->record(region_size);
}

}
