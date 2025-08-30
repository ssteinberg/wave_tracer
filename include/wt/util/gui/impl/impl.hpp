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
#include <memory>
#include <atomic>
#include <optional>

#include <chrono>

#include <wt/wt_context.hpp>
#include <wt/version.hpp>

#include <wt/scene/loader/bootstrap.hpp>
#include <wt/scene/scene_renderer.hpp>
#include <wt/sensor/response/RGB.hpp>

#include <wt/util/gui/utils.hpp>
#include <wt/util/gui/dependencies.hpp>
#include <wt/util/gui/impl/scene_info.hpp>
#include <wt/util/gui/impl/perf_stat.hpp>
#include <wt/util/gui/gui.hpp>

#include <wt/util/logger/logger.hpp>
#include <wt/util/logger/string_ostream.hpp>
#include <wt/util/format/utils.hpp>

#include "common.hpp"


namespace wt::gui {

using perf_stats_t = std::vector<perf_stat_t>;

struct impl_t {
    impl_t(wt_context_t& ctx,
           std::unique_ptr<gui_t::bootstrap_data_t> scene_bootstrapper,
           gui_t::renderer_results_callback_t write_out_render_results) 
        : ctx(ctx),
          write_out_render_results_callback(std::move(write_out_render_results)),
          scene_bootstrapper(std::move(scene_bootstrapper)),
          lm(std::make_unique<std::mutex>()),
          cout(sout, *lm), cwrn(sout, *lm), cerr(sout, *lm),
          wtversion_string("wave_tracer " + wt_version_t{}.short_version_string())
    {
        // scene loading future
        scene_bootstrap_future = std::async(std::launch::async, [this](){ this->scene_bootstrapper->ptr->wait(); });

        // attach a sink to standard outputs
        // each stream writes lines to sout, marking each line with a log_type_e to indicate source.
        logger::termcolour::set_colourized(cout);
        logger::termcolour::set_colourized(cwrn);
        logger::termcolour::set_colourized(cerr);
        logger::cout.add_ostream(cout, loglevel);
        logger::cwarn.add_ostream(cwrn, loglevel);
        logger::cerr.add_ostream(cerr, loglevel);
        // don't print to cout console
        logger::cout.set_sout_level(verbosity_e::quiet);

        reset_preview_controls();

        const auto& scene_name = this->scene_bootstrapper->ptr->get_scene_loader()->get_name();
        wt::logger::cout(wt::verbosity_e::normal) << "loading scene \'" << scene_name << "\'..." << '\n';
    }
    ~impl_t() {
        logger::cout.remove_ostream(cout);
        logger::cwarn.remove_ostream(cwrn);
        logger::cerr.remove_ostream(cerr);
    }

    void init(const wt_context_t& ctx);
    void deinit();
    void new_frame();
    void render();

    void load_fonts();

    inline void set_sout_verbosity(verbosity_e l) noexcept {
        loglevel = l;
        logger::cout.set_ostream_level(cout, loglevel);
        logger::cwarn.set_ostream_level(cwrn, loglevel);
        logger::cerr.set_ostream_level(cerr, loglevel);
    }

    void print_summary() {
        wt::logger::cout(wt::verbosity_e::info)
            << std::format("{}  |  {:L} emitters  |  {:L} shapes",
                           scene->description(), scene->sensors().size(), scene->shapes().size())
            << '\n';
        wt::logger::cout(wt::verbosity_e::info)
            << std::format("{}  |  {:L} triangles  |  {:L} nodes", 
                           ads->description(), ads->triangles_count(), ads->nodes_count())
            << '\n';
    }

    inline auto gui_title() const {
        return "wave_tracer  —  " + scene_bootstrapper->ptr->get_scene_loader()->get_name();
    }

    inline const auto& get_scene_bootstrapper() const noexcept {
        return scene_bootstrapper;
    }
    inline bool is_scene_loading_done() const noexcept {
        assert(!!scene_bootstrap_future);
        using namespace std::chrono_literals;
        return scene_bootstrap_future->wait_for(0ms) == std::future_status::ready;
    }

    inline void create_scene() {
        assert(!!scene_bootstrap_future);
        scene_bootstrap_future->get();

        // scene loaded
        scene = std::move(*scene_bootstrapper->ptr).get_scene();
        ads   = std::move(*scene_bootstrapper->ptr).get_ads();
        scene_bootstrapper = {};
        scene_bootstrap_future = {};
        scene_updated = true;

        // build scene info
        if (!scene->sensors().empty()) {
            const auto& sensor = *scene->sensors().begin()->get_sensor();
            scene_info = build_scene_info("", std::nullopt, scene->description(), sensor);
            ads_info   = build_scene_info("", std::nullopt, ads->description(), sensor);
        }

        this->RGB_response_function = nullptr;
        if (!scene->sensors().empty()) {
            // extract RGB response function, if any
            const auto* sensor_response = scene->sensors().begin()->get_sensor()->sensor_response();
            if (const auto* rgb = dynamic_cast<const sensor::response::RGB_t*>(sensor_response); rgb)
                this->RGB_response_function = rgb;

            // query tonemapping operator, and try to match GUI visualization mode
            if (const auto tm = sensor_response->get_tonemap(); tm) {
                // dB?
                if (tm->get_tonemapping_op() == sensor::response::tonemap_e::dB) {
                    const auto db_range = tm->get_dB_range();
                    this->db_range = this->db_range_default = { db_range.min, db_range.max };
                    this->set_mode_db();
                }

                // colourmap
                const auto& cm = format::tolower(tm->get_colourmap_name());
                for (int id=0; id<colourmap_names.size(); ++id) {
                    if (cm == colourmap_names[id]) {
                        this->colourmap_id = id;
                        break;
                    }
                }
            }
        }
    }

    inline void start_rendering(const gui_t* gui) noexcept {
        assert(!has_rendering_started());

        rendering_start_time = std::chrono::high_resolution_clock::now();
        rendering_elapsed_time = {};

        scene::render_opts_t opts = {};
        opts.previewer = gui;

        // start rendering thread
        scene_renderer = std::make_unique<scene_renderer_t>(*scene, ctx, *ads, std::launch::async, std::move(opts));
    }
    inline bool has_rendering_started() const noexcept { return !!scene_renderer; }
    inline auto rendering_status() const noexcept {
        return scene_renderer->rendering_status();
    }
    inline bool is_scene_renderer_done() const noexcept {
        assert(has_rendering_started());
        using namespace std::chrono_literals;
        return scene_renderer->wait_for(0ms) == std::future_status::ready;
    }
    inline void process_rendering_result() noexcept {
        assert(has_rendering_started());
        const auto& render_result = scene_renderer->get();

        rendering_elapsed_time = std::chrono::high_resolution_clock::now() - rendering_start_time;

        // write out
        write_results(render_result, false);
    }
    inline void write_results(const scene::render_result_t& results, bool intermediate) const {
        write_out_render_results_callback(*scene, *ads, results, intermediate);
    }

    inline void renderer_pause() const noexcept {
        scene_renderer->interrupt(std::make_unique<wt::scene::interrupts::pause_t>());
    }
    inline void renderer_resume() const noexcept {
        scene_renderer->interrupt(std::make_unique<wt::scene::interrupts::resume_t>());
    }
    inline void renderer_toggle_pauseresume() const noexcept {
        if (!has_rendering_started()) return;

        const auto& state = rendering_status().state;
        if (state == rendering_state_t::rendering)
            renderer_pause();
        else if (state == rendering_state_t::paused || state == rendering_state_t::pausing)
            renderer_resume();
    }
    inline void capture_intermediate() const noexcept {
        if (!has_rendering_started()) return;

        scene_renderer->interrupt(std::make_unique<wt::scene::interrupts::capture_intermediate_t>(
                [this](const scene::render_result_t& results) { this->write_results(results, true); }
            ));
    }

    // this should be called from background thread that process previews
    inline void on_new_preview(const preview_bitmap_t& pbmp) const noexcept {
        if constexpr (draw_histogram || do_perf_stats)
            std::atomic_thread_fence(std::memory_order_acquire);

        if constexpr (draw_histogram) {
            // create histogram
            if (histogram_shown)
                new_image_histogram = std::make_shared<histogram_t<>>(pbmp);
            else
                new_image_histogram = nullptr;
        }

        if constexpr (do_perf_stats) {
            // build perf stats
            if (perf_stats_open)
                new_perf_stats = std::make_shared<perf_stats_t>(build_perf_stats());
        }
    }
    // this should be called from background thread that process previews
    inline void push_new_preview(std::shared_ptr<preview_bitmap_t>&& pi, f_t spe_completed) const noexcept {
        on_new_preview(*pi);

        preview_surface = std::move(pi);
        in_spe_completed = spe_completed;
    }
    // this should be called from background thread that process previews
    inline void push_new_preview(std::shared_ptr<preview_bitmap_polarimetric_t>&& pi, f_t spe_completed) const noexcept {
        on_new_preview((*pi)[0]);

        preview_surface_polarimetric = std::move(pi);
        in_spe_completed = spe_completed;
    }

    // called from gui thread to consume a new preview update
    inline auto update_preview() noexcept {
        auto pi = preview_surface.load(std::memory_order_relaxed);
        while (!preview_surface.compare_exchange_weak(
                pi, std::shared_ptr<preview_bitmap_t>{},
                std::memory_order_release, std::memory_order_relaxed)) {}
        auto pip = preview_surface_polarimetric.load(std::memory_order_relaxed);
        while (!preview_surface_polarimetric.compare_exchange_weak(
                pip, std::shared_ptr<preview_bitmap_polarimetric_t>{},
                std::memory_order_release, std::memory_order_relaxed)) {}

        if (!pi && !pip)
            return;
        
        // on first update, recentre and rezoom image
        if (!preview_gl_image && !preview_gl_image_polarimetric)
            should_recentre_image = true;
        spe_completed = in_spe_completed.load(std::memory_order::relaxed);

        if (pi) {
            preview_gl_image = gui::gl_image_t(std::move(pi));
            preview_gl_image_polarimetric = {};
        } else if (pip) {
            preview_gl_image_polarimetric = gui::gl_images_t(std::move(pip));
            preview_gl_image = {};
        }

        // update histogram
        if constexpr (draw_histogram) {
            auto h = new_image_histogram.load(std::memory_order_relaxed);
            while (!new_image_histogram.compare_exchange_weak(
                    h, std::shared_ptr<histogram_t<>>{},
                    std::memory_order_release, std::memory_order_relaxed))
            {}
            if (h)
                image_histogram = std::move(h);
        }

        // update perf stats
        if constexpr (do_perf_stats) {
            auto ps = new_perf_stats.load(std::memory_order_relaxed);
            while (!new_perf_stats.compare_exchange_weak(
                    ps, std::shared_ptr<perf_stats_t>{},
                    std::memory_order_release, std::memory_order_relaxed))
            {}
            if (ps) {
                perf_stats = std::move(*ps);
                last_perf_stats_update = std::chrono::steady_clock::now();
            }
        }
    }

    // should be called from gui thread on opening stats tab, might block.
    void update_perf_stats_if_stale() {
        if constexpr (!do_perf_stats)
            return;

        using namespace std::chrono_literals;
        if (std::chrono::steady_clock::now() - last_perf_stats_update > 5s) {
            // if more than several seconds have passed since last update, update synchronously
            perf_stats = build_perf_stats();
            last_perf_stats_update = std::chrono::steady_clock::now();
        }
    }

    inline bool has_preview() const noexcept { return preview_gl_image || preview_gl_image_polarimetric; }
    inline bool is_polarimetric_preview() const noexcept {
        return !!preview_gl_image_polarimetric;
    }

    inline auto is_rgb_preview() const noexcept {
        if (!RGB_response_function)
            return false;
        if (is_polarimetric_preview()) {
            if (preview_gl_image_polarimetric.images)
                return preview_gl_image_polarimetric.images->front().components()>=3;
        } else {
            if (preview_gl_image.image)
                return preview_gl_image.image->components()>=3;
        }
        return false;
    }


    wt_context_t& ctx;
    bool scene_updated = false;
    std::unique_ptr<wt::scene_t> scene;
    std::unique_ptr<wt::ads::ads_t> ads;
    std::unique_ptr<scene_info_t> scene_info;
    std::unique_ptr<scene_info_t> ads_info;
    std::unique_ptr<scene_renderer_t> scene_renderer;
    const sensor::response::RGB_t* RGB_response_function;

    gl_image_t icon;
    ImFont* mono_font;

    perf_stats_t perf_stats;
    std::chrono::steady_clock::time_point last_perf_stats_update;

    gl_image_t preview_gl_image;
    gl_images_t preview_gl_image_polarimetric;
    f_t spe_completed;

    int pol_mode_id = 0, pol_mode_filter_mode = 0;
    float pol_LP_filter_angle = 0;
    vec4_t pol_stokes_filter = { 1,0,0,0 };

    bool histogram_shown = true, perf_stats_open = false;
    std::shared_ptr<histogram_t<>> image_histogram;

    std::array<gl_image_t, ImGuiTexInspect::colourmaps.size()> colourmap_legend_bars;
    
    bool should_recentre_image = false, should_fit_image = false, should_fill_image = false;
    preview_mode_e preview_mode = preview_mode_e::gamma;
    bool preview_tooltips = false, preview_annotations = true;
    float exposure, gamma;
    bool srgb_gamma = true;
    vec2<float> db_range, db_range_default = { -100,0 };
    int colourmap_id = 0;
    float fc_min = 0, fc_max = 1;
    int fc_channel = 4;

    inline bool lock_linear_fc() const noexcept { return is_polarimetric_preview() && 4<=pol_mode_id && pol_mode_id<=8; }
    inline bool custom_fc() const noexcept      { return is_polarimetric_preview() && 7<=pol_mode_id && pol_mode_id<=8; }
    inline bool mirrored_fc() const noexcept    { return is_polarimetric_preview() && 1<=pol_mode_id && pol_mode_id<=3; }

    inline auto current_preview_mode() const noexcept {
        return lock_linear_fc() ? preview_mode_e::fc : preview_mode;
    }

    inline void set_mode_linear() noexcept { preview_mode = preview_mode_e::linear; }
    inline void set_mode_gamma() noexcept  { preview_mode = preview_mode_e::gamma; }
    inline void set_mode_db() noexcept     { preview_mode = preview_mode_e::db; }
    inline void set_mode_fc() noexcept     { preview_mode = preview_mode_e::fc; }
    inline void inc_exposure(float scale=1) noexcept {
        if (preview_mode==preview_mode_e::linear || preview_mode==preview_mode_e::gamma)
            exposure+=.01f*scale;
        if (exposure==0) exposure=0;
    }
    inline void dec_exposure(float scale=1) noexcept {
        if (preview_mode==preview_mode_e::linear || preview_mode==preview_mode_e::gamma)
            exposure-=.01f*scale;
        if (exposure==0) exposure=0;
    }
    inline void inc_gamma(float scale=1) noexcept {
        if (preview_mode==preview_mode_e::gamma && !srgb_gamma)
            gamma+=.025f*scale;
        if (gamma==0) gamma=0;
    }
    inline void dec_gamma(float scale=1) noexcept {
        if (preview_mode==preview_mode_e::gamma && !srgb_gamma)
            gamma-=.025f*scale;
        if (gamma==0) gamma=0;
    }
    inline void toggle_gamma_srgb() noexcept {
        if (preview_mode==preview_mode_e::gamma)
            srgb_gamma = !srgb_gamma;
    }

    inline void reset_preview_controls() noexcept {
        exposure = 0; gamma = 2.2; srgb_gamma = true;
        db_range = db_range_default;
        fc_min=0; fc_max=1;
        pol_mode_id = 0;
    }

    gui::gui_state_t state = gui::gui_state_t::loading;
    std::chrono::high_resolution_clock::time_point rendering_start_time;
    std::chrono::high_resolution_clock::duration rendering_elapsed_time;

    bool show_logbox = open_logbox_by_default, show_sidebar = open_sidebar_by_default;

    logbox_ctx_t logbox;
    verbosity_e loglevel = default_sout_verbosity;
    std::vector<std::pair<int, std::string>> sout;
    std::size_t seen_sout_lines = 0;
    bool should_scroll_log_to_bottom = false;

    SDL_Window* window;
    ImGuiIO* io;

    bool main_layout_configured = false;
    bool about_popup_open = false;

    inline float status_bar_height() const noexcept {
        return ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y*2 + 7;
    }

private:
    gui_t::renderer_results_callback_t write_out_render_results_callback;

    std::unique_ptr<gui_t::bootstrap_data_t> scene_bootstrapper;
    std::optional<std::future<void>> scene_bootstrap_future;

    SDL_GLContext gl_context;

    std::unique_ptr<std::mutex> lm;
    logger::string_ostream<log_type_e::cout> cout;
    logger::string_ostream<log_type_e::cwarn> cwrn;
    logger::string_ostream<log_type_e::cerr> cerr;

    mutable std::atomic<std::shared_ptr<preview_bitmap_t>> preview_surface;
    mutable std::atomic<std::shared_ptr<preview_bitmap_polarimetric_t>> preview_surface_polarimetric;
    mutable std::atomic<std::shared_ptr<histogram_t<>>> new_image_histogram;
    mutable std::atomic<f_t> in_spe_completed;

    mutable std::atomic<std::shared_ptr<perf_stats_t>> new_perf_stats;

public:
    const std::string wtversion_string;
};

}
