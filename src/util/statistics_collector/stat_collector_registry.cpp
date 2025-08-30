/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 * Authors:  Umut Emre, Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#include <algorithm>
#include <wt/util/statistics_collector/stat_collector.hpp>
#include <wt/util/statistics_collector/stat_collector_registry.hpp>

using namespace wt;
using namespace wt::stats;


std::vector<std::unique_ptr<stat_collector_t>> stat_collector_registry_t::get_collectors() const {
    std::unordered_map<std::string, std::vector<stat_collector_t*>> name_to_counters{};
    for (const auto& [key, collector] : collectors) {
        name_to_counters[collector->name].push_back(collector.get());
    }

    std::vector<std::unique_ptr<stat_collector_t>> result;
    for (const auto& [name, stats] : name_to_counters) {
        if (stats.size() <= 0) {
            continue;
        }
        
        auto accumulator = std::accumulate(stats.begin(), stats.end(), stats[0]->zero(), [](auto acc, auto stat) {
            *acc += *stat;
            return acc;
        });
        result.emplace_back(std::move(accumulator));
    }

    // not strictly necessary but nicer to have deterministic order when printing
    std::ranges::sort(result, [](const auto& a, const auto& b) { return a->name < b->name; });

    return result;
}
