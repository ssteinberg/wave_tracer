/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <memory>
#include <mutex>

#include <wt/math/common.hpp>
#include <wt/bitmap/bitmap.hpp>

#include <wt/scene/loader/bootstrap.hpp>
#include <wt/scene/render_results.hpp>

#include <wt/util/preview/preview_interface.hpp>

namespace wt {

/**
 * @brief Graphical user interface that handles scene loading and rendering preview.
 */
class gui_t final : public preview_interface_t {
public:
    /**
     * @brief Callback that should process renderer results.
     *        4th boolean parameter indicates if the results are intermediate results.
     *        Callback may be called from different threads.
     */
    using renderer_results_callback_t =
        std::function<void(const scene_t&, const ads::ads_t&, const scene::render_result_t&, bool)>;

private:
    wt_context_t& context;
    std::shared_ptr<void> ptr;

public:
    struct bootstrap_data_t {
        std::unique_ptr<scene::scene_bootstrap_generic_t> ptr;

        std::atomic<f_t> scene_loading_progress = 0;
        std::atomic<f_t> resource_loading_progress = 0;
        std::atomic<f_t> ads_construction_progress = 0;
        std::shared_ptr<std::string> ads_construction_status;
        mutable std::mutex ads_construction_status_mutex;
    };

private:
    template <std::derived_from<scene::scene_bootstrap_generic_t> Bootstrap>
    static auto create_scene_bootstrap(
            wt_context_t& context,
            const std::filesystem::path& scene_path,
            const scene::loader::defaults_defines_t& scene_loader_defines) {
        // scene name: parent directory + scene file name
        auto scene_name = 
            (scene_path.has_parent_path() ?
                scene_path.parent_path().filename()/scene_path.filename() :
                scene_path.filename()
            ).string();

        auto data = std::make_unique<bootstrap_data_t>();

        // progress tracker
        auto pb_prog = wt::scene::bootstrap_progress_callback_t{
            .scene_loading_progress_update = [v=data.get()](wt::f_t p) noexcept {
                v->scene_loading_progress = p;
            },
            .resources_loading_progress_update = [v=data.get()](wt::f_t p) noexcept {
                v->resource_loading_progress = p;
            },
            .ads_progress_update = [v=data.get()](wt::f_t p) noexcept {
                v->ads_construction_progress = p;
            },
            .ads_construction_status_update = [v=data.get()](std::string status) noexcept {
                v->ads_construction_status = std::make_shared<std::string>(std::move(status));
            },
            .on_finish = []() noexcept {},
        };
        // create bootstrapper
        data->ptr = std::make_unique<Bootstrap>(
                std::move(scene_name),
                scene_path,
                context,
                scene_loader_defines,
                std::move(pb_prog));

        return data;
    }

public:
    gui_t(wt_context_t& ctx,
          std::unique_ptr<bootstrap_data_t> scene_bootstrapper,
          renderer_results_callback_t write_out_render_results);

    /**
     * @brief Launches the GUI and takes control of the scene and context
     */
    template <typename Bootstrap>
    static auto launch(
            wt_context_t& context,
            const std::filesystem::path& scene_path,
            const scene::loader::defaults_defines_t& scene_loader_defines,
            renderer_results_callback_t write_out_render_results) {
        return std::make_unique<gui_t>(
                context,
                create_scene_bootstrap<Bootstrap>(context, scene_path, scene_loader_defines),
                std::move(write_out_render_results));
    }
    virtual ~gui_t() noexcept {}

    // TODO: provide user control over preview polling intervals.

    /**
     * @brief Indicates a desired minimal interval from clients calling update().
     */
    // [[nodiscard]] std::chrono::milliseconds preview_update_interval() const noexcept override;
    /**
     * @brief Indicates a desired rate limiting factor from clients calling update().
     */
    [[nodiscard]] unsigned preview_update_rate_limit_factor() const noexcept override { return 4; }

    /**
     * @brief Updates the preview image. Can be called from any thread.
     */
    void update(const std::string& preview_id, sensor::developed_scalar_film_t<2>&& surface,
                const f_t spp_completed,
                const sensor::response::tonemap_t* tonemap = nullptr) const override;

    /**
     * @brief Updates the preview image (polarimetric input). Can be called from any thread.
     */
    void update(const std::string& preview_id, sensor::developed_polarimetric_film_t<2>&& surface,
                const f_t spp_completed,
                const sensor::response::tonemap_t* tonemap = nullptr) const override;

    [[nodiscard]] bool polarimetric_preview() const noexcept override;
    [[nodiscard]] bool available() const noexcept override { return true; }
};

}
