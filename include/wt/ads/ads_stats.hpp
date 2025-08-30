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

#include <utility>

#include <wt/math/intersect/ray.hpp>
#include <wt/math/intersect/cone.hpp>

#include <wt/util/statistics_collector/stat_collector_registry.hpp>
#include <wt/util/statistics_collector/stat_counter.hpp>
#include <wt/util/statistics_collector/stat_counter_event.hpp>
#include <wt/util/statistics_collector/stat_histogram.hpp>
#include <wt/util/statistics_collector/stat_timings.hpp>

namespace wt::ads_stats {

#if defined(_additional_ads_stats)
static constexpr auto additional_ads_counters = true;
#else
static constexpr auto additional_ads_counters = false;
#endif

namespace detail {

using namespace wt::stats;

struct ads_stat_counters_t {
    stat_counter_event_t<3>* ray_cast_event_counter = 
        stat_collector_registry_t::instance().make_collector<stat_counter_event_t<3>>(
            "(ADS) casts ray",
            std::array<std::string,3>{ "hit", "miss", "esc" }
        );
    stat_counter_event_t<4>* cone_cast_event_counter = 
        stat_collector_registry_t::instance().make_collector<stat_counter_event_t<4>>(
            "(ADS) casts cone",
            std::array<std::string,4>{ "1 hit", ">1 hit", "miss", "esc" }
        );

    stat_timings_t* ray_cast_timings = additional_ads_counters ?
        stat_collector_registry_t::instance().make_collector<stat_timings_t>("(ADS) timings ray") :
        nullptr;
    stat_timings_t* cone_cast_timings = additional_ads_counters ?
        stat_collector_registry_t::instance().make_collector<stat_timings_t>("(ADS) timings cone") :
        nullptr;

    stat_timings_t* shadow_ray_cast_timings = additional_ads_counters ?
        stat_collector_registry_t::instance().make_collector<stat_timings_t>("(ADS) timings shadow ray") :
        nullptr;
    stat_timings_t* shadow_cone_cast_timings = additional_ads_counters ?
        stat_collector_registry_t::instance().make_collector<stat_timings_t>("(ADS) timings shadow cone") :
        nullptr;

    stat_histogram_t<256>* tris_returned_per_query = 
        stat_collector_registry_t::instance().make_collector<stat_histogram_t<256>>("(ADS) tris per cone", 1);

    stat_counter_event_t<6>* intersection_tests_counter = additional_ads_counters ?
        stat_collector_registry_t::instance().make_collector<stat_counter_event_t<6>>(
            "(ADS) tests intersection",
            std::array<std::string,6>{ "8×ray-tri", "8×ray-box", "cone-box", "cone-tri" }
        ) :
        nullptr;
    stat_counter_event_t<2>* shadow_tests_counter = additional_ads_counters ?
        stat_collector_registry_t::instance().make_collector<stat_counter_event_t<2>>(
            "(ADS) tests shadow",
            std::array<std::string,2>{ "ray-tri", "cone-tri" }
        ) :
        nullptr;

    stat_histogram_t<127>* ray_nodes_visited = additional_ads_counters ?
        stat_collector_registry_t::instance().make_collector<stat_histogram_t<127>>("(ADS) nodes visited (ray)", 1) :
        nullptr;
    stat_histogram_t<127>* cone_nodes_visited = additional_ads_counters ?
        stat_collector_registry_t::instance().make_collector<stat_histogram_t<127>>("(ADS) nodes visited (cone)", 1) :
        nullptr;
    stat_counter_event_t<3>* cone_node_types_visited = additional_ads_counters ?
        stat_collector_registry_t::instance().make_collector<stat_counter_event_t<3>>(
            "(ADS) node types (cone)",
            std::array<std::string,3>{ "internal", "leaf", "subtree" }
        ) :
        nullptr;
};

}

inline thread_local detail::ads_stat_counters_t ads_stats_counters{};

inline void on_ray_cast_event(bool hit, bool escaped, bool shadow,
                              std::chrono::high_resolution_clock::time_point start,
                              int nodes_visited) noexcept {
    ads_stats_counters.ray_cast_event_counter->record(escaped ? 2 : hit ? 0 : 1);
    if constexpr (additional_ads_counters)
        (*(shadow ? ads_stats_counters.shadow_ray_cast_timings : ads_stats_counters.ray_cast_timings))
            .record(std::chrono::high_resolution_clock::now() - start);
    if constexpr (additional_ads_counters)
        ads_stats_counters.ray_nodes_visited->increment_count_of(nodes_visited);
}

inline void on_cone_cast_event(bool hit, bool escaped, std::size_t tris,
                               std::chrono::high_resolution_clock::time_point start,
                               int internal_nodes_visited, int leaf_nodes_visited, int subtrees_visited) noexcept {
    const auto nodes_visited = internal_nodes_visited + leaf_nodes_visited + subtrees_visited;
    if (hit)
        ads_stats_counters.tris_returned_per_query->increment_count_of(tris);
    ads_stats_counters.cone_cast_event_counter->record(escaped ? 3 : hit ? (tris==1 ? 0 : 1) : 2);
    if constexpr (additional_ads_counters) {
        (*ads_stats_counters.cone_cast_timings).record(std::chrono::high_resolution_clock::now() - start);
        ads_stats_counters.cone_nodes_visited->increment_count_of(nodes_visited);
        ads_stats_counters.cone_node_types_visited->record(0, internal_nodes_visited);
        ads_stats_counters.cone_node_types_visited->record(1, leaf_nodes_visited);
        ads_stats_counters.cone_node_types_visited->record(2, subtrees_visited);
    }
}
inline void on_shadow_cone_cast_event(bool hit, bool escaped,
                                      std::chrono::high_resolution_clock::time_point start,
                                      int internal_nodes_visited, int leaf_nodes_visited, int subtrees_visited) noexcept {
    const auto nodes_visited = internal_nodes_visited + leaf_nodes_visited + subtrees_visited;
    ads_stats_counters.cone_cast_event_counter->record(escaped ? 3 : hit ? 0 : 2);
    if constexpr (additional_ads_counters) {
        (*ads_stats_counters.shadow_cone_cast_timings).record(std::chrono::high_resolution_clock::now() - start);
        ads_stats_counters.cone_nodes_visited->increment_count_of(nodes_visited);
        ads_stats_counters.cone_node_types_visited->record(0, internal_nodes_visited);
        ads_stats_counters.cone_node_types_visited->record(1, leaf_nodes_visited);
        ads_stats_counters.cone_node_types_visited->record(2, subtrees_visited);
    }
}

inline void log_cone_query_termina(bool hit, bool escaped, std::size_t tris,
                               std::chrono::high_resolution_clock::time_point start,
                               int nodes_visited) noexcept {
    if (hit)
        ads_stats_counters.tris_returned_per_query->increment_count_of(tris);
    ads_stats_counters.cone_cast_event_counter->record(escaped ? 3 : hit ? (tris==1 ? 0 : 1) : 2);
    if constexpr (additional_ads_counters) {
        (*ads_stats_counters.cone_cast_timings).record(std::chrono::high_resolution_clock::now() - start);
        ads_stats_counters.cone_nodes_visited->increment_count_of(nodes_visited);
    }
}

inline void on_ray_aabb_8w_test() noexcept {
    if constexpr (additional_ads_counters)
        ads_stats_counters.intersection_tests_counter->record(1);
}

/**
 * @brief Wrapper around intersect_ray_tri that collects performance stats.
 */
template <typename... Ts>
inline auto intersect_ray_tri_8w(Ts&&... ts) noexcept {
    if constexpr (additional_ads_counters)
        ads_stats_counters.intersection_tests_counter->record(0);
    return intersect::intersect_ray_tri(std::forward<Ts>(ts)...);
}

/**
 * @brief Wrapper around test_ray_tri that collects performance stats.
 */
template <typename... Ts>
inline auto test_ray_tri_8w(Ts&&... ts) noexcept {
    if constexpr (additional_ads_counters)
        ads_stats_counters.shadow_tests_counter->record(0);
    return intersect::test_ray_tri(std::forward<Ts>(ts)...);
}

/**
 * @brief Wrapper around test_cone_aabb that collects performance stats.
 */
template <typename... Ts>
inline bool test_cone_aabb(Ts&&... ts) noexcept {
    if constexpr (additional_ads_counters)
        ads_stats_counters.intersection_tests_counter->record(2);
    return intersect::test_cone_aabb(std::forward<Ts>(ts)...);
}

/**
 * @brief Wrapper around intersect_cone_tri that collects performance stats.
 */
template <typename... Ts>
inline std::optional<intersect::intersect_cone_tri_ret_t> intersect_cone_tri(Ts&&... ts) noexcept {
    if constexpr (additional_ads_counters)
        ads_stats_counters.intersection_tests_counter->record(3);
    return intersect::intersect_cone_tri(std::forward<Ts>(ts)...);
}

/**
 * @brief Wrapper around test_cone_tri that collects performance stats.
 */
template <typename... Ts>
inline bool test_cone_tri(Ts&&... ts) noexcept {
    if constexpr (additional_ads_counters)
        ads_stats_counters.shadow_tests_counter->record(5);
    return intersect::test_cone_tri(std::forward<Ts>(ts)...);
}

}  // namespace wt::ads_stats
