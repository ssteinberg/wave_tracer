/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <optional>
#include <filesystem>

#include <wt/math/quantity/defs.hpp>

namespace wt {

class preview_interface_t;

class preview_window_t;

namespace thread_pool { class tpool_t; }

/**
 * @brief Used for logging and output verbosity adjustments.
 */
enum class verbosity_e : std::int8_t {
    quiet  = -9,
    important  = -5,
    normal = 0,
    info   = 5,
    debug  = 9,
};

/**
 * @brief Holds configurations and pointers to global resources used by wave_tracer for a single render.
 */
struct wt_context_t {
    length_t default_scale_for_imported_mesh_positions = 1*u::m;


    bool renderer_force_ray_tracing = false;

    std::uint32_t renderer_block_size = 24;
    std::uint32_t renderer_samples_per_block = 8;


	std::filesystem::path scene_data_path, output_path;


    /** @brief Thread pool */
    wt::thread_pool::tpool_t* threadpool;


    /**
     * @brief Resolves a resource path. 
     *        If the path is relative, first searches in the ``scene_data_path`` directory (unless ``search_in_scene_data`` is false), then searches in work directory (``./``) directory, and finally in parent of work directory (``../``).
     *        Returns ``std::nullopt`` if file was not found.
     */
    [[nodiscard]] inline std::optional<std::filesystem::path> resolve_path(
            std::filesystem::path path,
            bool search_in_scene_data = true) const noexcept {
        if (path.is_absolute())
            return path;

        if (search_in_scene_data) {
            auto path_in_scene = scene_data_path / path;
            if (std::filesystem::is_regular_file(path_in_scene))
                return path_in_scene;
        }

        if (std::filesystem::is_regular_file(path))
            return path;
        if (std::filesystem::is_regular_file(".." / path))
            return ".." / path;

        // not found
        return std::nullopt;
    }
};

}
