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

#include <wt/util/statistics_collector/stat_collector_registry.hpp>
#include "wt/util/statistics_collector/stat_counter.hpp"
#include "wt/util/statistics_collector/stat_counter_event.hpp"

namespace wt::ads_constructor_stats {

#if defined(_additional_ads_stats)
static constexpr auto additional_stats = true;
#else
static constexpr auto additional_stats = false;
#endif

namespace detail {

using namespace wt::stats;

struct ads_construction_stat_timers_t {
    stat_counter_t<f_t>* sah_cost = additional_stats ? stat_collector_registry_t::instance().make_collector<stat_counter_t<f_t>>("(ADS CTOR) SAH cost") : nullptr;
    
    stat_counter_t<std::size_t>* tri_count = additional_stats ? stat_collector_registry_t::instance().make_collector<stat_counter_t<std::size_t>>("(ADS CTOR) tri count") : nullptr;
    
    stat_counter_t<std::size_t>* max_depth = additional_stats ? stat_collector_registry_t::instance().make_collector<stat_counter_t<std::size_t>>("(ADS CTOR) max depth") : nullptr;

    stat_counter_event_t<2>* node_counts = additional_stats ? 
        stat_collector_registry_t::instance().make_collector<stat_counter_event_t<2>>(
            "(ADS CTOR) node count",
            std::array<std::string,2>{ "interior", "leaf" }
        ) : nullptr;
};

}  // namespace detail

inline detail::ads_construction_stat_timers_t stats{};

inline void set_sah_cost(f_t cost) {
    if constexpr (!additional_stats) return;
    stats.sah_cost->set(cost); 
}

inline void record_node_amounts(std::size_t interior_count, std::size_t leaf_count) {
    if constexpr (!additional_stats) return;
    stats.node_counts->set_amount(0, interior_count); 
    stats.node_counts->set_amount(1, leaf_count); 
}

inline void record_tri_count(std::size_t count) {
    if constexpr (!additional_stats) return;
    stats.tri_count->set(count); 
}

inline void record_max_depth(std::size_t depth) {
    if constexpr (!additional_stats) return;
    stats.max_depth->set(depth); 
}

}  // namespace wt::ads_constructor_stats