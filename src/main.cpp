/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#ifndef WIN32
#include <signal.h>
#endif

#include <string>
#include <chrono>
#include <utility>
#include <stdexcept>

#include <filesystem>
#include <sstream>
#include <format>

#include <future>

#include <wt/sensor/sensor/film_backed_sensor.hpp>

#include <wt/version.hpp>
#include <wt/wt_context.hpp>

#include <wt/bitmap/bitmap.hpp>
#include <wt/bitmap/write2d.hpp>

#include <wt/scene/loader/bootstrap.hpp>
#include <wt/scene/loader/xml/loader.hpp>
#include <wt/ads/bvh8w/bvh8w_constructor.hpp>
#include <wt/scene/scene_renderer.hpp>

#include <wt/util/logger/logger.hpp>
#include <wt/util/logger/file_log.hpp>
#include <wt/util/format/enum.hpp>
#include <wt/util/format/chrono.hpp>
#include <wt/util/thread_pool/tpool.hpp>

#include <wt/util/preview/preview_tev.hpp>
#include <wt/util/gui/gui.hpp>
#include <wt/util/font_renderer/font_renderer.hpp>

#include <wt/util/statistics_collector/stat_collector_registry.hpp>

#define throw(...)
#include <ImfThreading.h>
#undef throw


#include <CLI/CLI.hpp>
#include <magic_enum/magic_enum_format.hpp>


// #define DEBUG_FP_EXCEPTIONS
#ifdef DEBUG_FP_EXCEPTIONS
#include <cfenv>
#endif



class argument_error : public std::runtime_error { using std::runtime_error::runtime_error; };


bool is_path_dir_writeable(const std::filesystem::path &path) {
    namespace fs = std::filesystem;

    if (!fs::exists(path) || !fs::is_directory(path)) return false;

    std::error_code ec;
    auto perms = fs::status(path, ec).permissions();
    return !ec &&
           ((perms & fs::perms::owner_write) != fs::perms::none ||
            (perms & fs::perms::group_write) != fs::perms::none ||
            (perms & fs::perms::others_write) != fs::perms::none);
}
bool is_path_file_readable(const std::filesystem::path &path) {
    namespace fs = std::filesystem;

    if (!fs::exists(path) || !fs::is_regular_file(path)) return false;

    std::error_code ec;
    auto perms = fs::status(path, ec).permissions();
    return !ec &&
           ((perms & fs::perms::owner_read) != fs::perms::none ||
            (perms & fs::perms::group_read) != fs::perms::none ||
            (perms & fs::perms::others_read) != fs::perms::none);
}
bool is_path_dir_readable(const std::filesystem::path &path) {
    namespace fs = std::filesystem;

    if (!fs::exists(path) || !fs::is_directory(path)) return false;

    std::error_code ec;
    auto perms = fs::status(path, ec).permissions();
    return !ec &&
           ((perms & fs::perms::owner_read) != fs::perms::none ||
            (perms & fs::perms::group_read) != fs::perms::none ||
            (perms & fs::perms::others_read) != fs::perms::none);
}


/* Performance statistics
 */

bool should_print_stats_to_stdout_on_exit = true;
bool should_write_stats_to_stdout_on_exit = false;

void print_stats_to_stdout() {
    using namespace wt::logger::termcolour;

    std::stringstream ss;
    wt::logger::termcolour::set_colourized(ss);
    for (const auto& stat : wt::stats::stat_collector_registry_t::instance().get_collectors())
        ss << *stat;
    
    // replace TAB with a single space for monospaced stdout
    auto stats_str = std::move(ss).str();
    std::ranges::replace(stats_str, '\t', ' ');

    wt::logger::cout(wt::verbosity_e::normal) 
        << '\n' 
        << "  " << bold << underline << "performance stats" 
        << reset << '\n'
        << stats_str
        << '\n';
}
void print_stats_to_file(const std::filesystem::path& path,
                         const wt::scene::render_result_t& render_result) {
    std::ofstream f(path);
    if (!f)
        throw std::runtime_error("Couldn't open \"" + path.string() + "\"");
    
    // also dump render times
    for (const auto& s : render_result.sensors)
        f << "(Scene) rendering time, "
          << "sensor" + s.first 
          << '\n';
    f << "(Scene) rendering time, total, "
      << std::format("{:%H:%M:%S}", render_result.render_elapsed_time)
      << '\n';

    // dump stat counters
    f << "name, bin, data" << '\n';
    for (const auto& stat : wt::stats::stat_collector_registry_t::instance().get_collectors())
        f << *stat;
}


/* Global resources
 */

wt::wt_context_t context;
std::unique_ptr<wt::thread_pool::tpool_t> threadpool;

std::unique_ptr<wt::scene_t> scene;
std::unique_ptr<wt::scene_renderer_t> scene_renderer;
std::unique_ptr<wt::ads::ads_t> ads;


/* Logging
 */

bool use_file_logging = true;

std::unique_ptr<wt::logger::file_logger_t> file_logger;

void print_summary() {
    wt::logger::cout(wt::verbosity_e::normal)
        << std::format("{}  |  {:L} emitters  |  {:L} shapes",
                        scene->description(), scene->sensors().size(), scene->shapes().size())
        << '\n';
    wt::logger::cout(wt::verbosity_e::normal)
        << std::format("{}  |  {:L} triangles  |  {:L} nodes", 
                        ads->description(), ads->triangles_count(), ads->nodes_count())
        << '\n';
}

void initialize_logs(wt::logger::log_verbosity_e filelog_verbosity, bool disable_progress_bars) {
    // disable progress bars, if requested
    if (disable_progress_bars)
        wt::logger::cout.disable_sout_progress_bars();

    // also log uncaught exceptions
    std::set_terminate([]() {
        {
            auto elog = wt::logger::cerr.log(wt::verbosity_e::important);
            elog << "terminate called after throwing an instance of ";
            try {
                std::rethrow_exception(std::current_exception());
            }
            catch (const std::exception &ex) {
                elog << typeid(ex).name() << ":  \"" << ex.what() << "\"\n";
            }
            catch (...) {
                elog << typeid(std::current_exception()).name() << '\n';
            }
            elog << "errno: " << errno << ": " << std::strerror(errno) << '\n';
        }

        if (file_logger)
            file_logger->flush();
        file_logger = nullptr;

        std::abort();
    });

    // file logger
    if (use_file_logging) {
        // open file log and bind standard output sinks
        const auto log_file_path = context.output_path / "log.txt";
        file_logger = std::make_unique<wt::logger::file_logger_t>(log_file_path);
        wt::logger::cout .add_ostream(file_logger->fout(),  filelog_verbosity);
        wt::logger::cwarn.add_ostream(file_logger->fwarn(), filelog_verbosity);
        wt::logger::cerr .add_ostream(file_logger->ferr(),  filelog_verbosity);

        wt::logger::cout(wt::verbosity_e::info) << "opened file log \'" << log_file_path << "\'." << '\n';
    }
}


/* Signal handlers
 */

bool terminate_program = false;

void sigint_handler(int) {
    if (scene_renderer)
        scene_renderer->interrupt(std::make_unique<wt::scene::interrupts::terminate_t>());
    terminate_program = true;
}


/* Rendering results writers
 */

bool watermark_results = true;

// writes out an EXR with additional metadata
void write_out(const std::filesystem::path& output_dir,
               const std::string& filename,
               const wt::bitmap::bitmap2d_t<float> &bm,
               const std::string& scene_name,
               const std::string& sensor_id,
               const std::size_t spe,
               const wt::bitmap::colour_encoding_type_e colour_encoding) {
    std::map<std::string,std::string> attributes;
    attributes["renderer"] = "wave_tracer " + wt::wt_version_t{}.short_version_string();
    attributes["scene"]    = scene_name;
    attributes["sensor"]   = sensor_id;
    attributes["samples"]  = std::format("{}", spe);

    using namespace wt::logger::termcolour;
    wt::logger::cout()
        << reset
        << "writing " 
        << bold << yellow
        << (output_dir / filename)      // this ignores extension
        << reset << yellow
        << "..." << '\n';

    if (colour_encoding == wt::bitmap::colour_encoding_type_e::linear)
        wt::bitmap::write_bitmap2d_exr(output_dir / (filename + ".exr"), bm, {}, attributes);
    else {
        auto bm16 = bm.convert_texels<std::uint16_t>();
        // TODO: indicate colourspace for bitmap writer
        wt::bitmap::write_bitmap2d_png(output_dir / (filename + ".png"), bm16);
    }
}

// watermarks a bitmap
inline void watermark(const wt::wt_context_t &ctx,
                      wt::bitmap::bitmap2d_t<float>& target) {
    if (watermark_results && target.width()>=256 && target.height()>=256) {
        wt::wt_version_t vrs;
        const auto vrs_string =
            std::format("{:d}",vrs.major()) + "." +
            std::format("{:d}",vrs.minor()) + "." +
            std::format("{:d}",vrs.patch());

        wt::font_renderer_t fr{ ctx, "ArchivoNarrow.ttf" };
        fr.render(std::string("wave_tracer ") + 
                  vrs_string, 
                  target, 
                  { 4, target.height()-4 }, 11.5, 
                  wt::font_renderer_t::anchor_t::bottom_left);
    }
}

inline void write_out_2d(
        const wt::wt_context_t &ctx,
        const std::filesystem::path& output_dir,
        const wt::sensor::sensor_t& sensor,
        const std::string& sensor_name,
        const std::string& scene_name,
        const wt::bitmap::bitmap2d_t<float>& developed_film,
        std::size_t spe_written,
        const wt::bitmap::colour_encoding_type_e colour_encoding,
        bool postprocess) {
    if (!postprocess) {
        // write out as is
        write_out(output_dir, sensor_name, developed_film,
                  scene_name, sensor_name, spe_written, colour_encoding);
    } else {
        // watermark
        auto developed = developed_film;
        watermark(ctx, developed);
        write_out(output_dir, sensor_name, developed,
                  scene_name, sensor_name, spe_written, colour_encoding);

        // and mask, if mask available
        if (const auto* fbsensor = dynamic_cast<const wt::sensor::film_backed_sensor_generic_t<2>*>(&sensor); fbsensor) {
            if (fbsensor->get_sensor_mask()) {
                const auto& mask =
                    fbsensor->get_sensor_mask()->create_mask(ctx, *ads, *scene, *fbsensor);
                // add alpha channel to developed image, and copy alpha from mask
                auto masked_img = developed_film.convert_texels<float>(developed_film.components() == 1 ?
                        wt::bitmap::pixel_layout_e::LA : wt::bitmap::pixel_layout_e::RGBA);
                wt::bitmap::copy_component(mask, masked_img, masked_img.components()-1);

                write_out(output_dir, sensor_name + "_masked", masked_img,
                        scene_name, sensor_name, spe_written, colour_encoding);
            }
        }
    }
}


// 1D scalar films
inline void write_out_films(
        const wt::wt_context_t &ctx,
        const std::filesystem::path& output_dir,
        const wt::sensor::sensor_t& sensor,
        const std::string& sensor_name,
        const std::string& scene_name,
        const wt::scene::developed_scalar_film_pair_t<1>& films,
        std::size_t spe_written) {
    // TODO
    throw std::runtime_error("not implemented!");
}
// 3D scalar films
inline void write_out_films(
        const wt::wt_context_t &ctx,
        const std::filesystem::path& output_dir,
        const wt::sensor::sensor_t& sensor,
        const std::string& sensor_name,
        const std::string& scene_name,
        const wt::scene::developed_scalar_film_pair_t<3>& films,
        std::size_t spe_written) {
    // TODO
    throw std::runtime_error("not implemented!");
}
// 1D polarimetric films
inline void write_out_films(
        const wt::wt_context_t &ctx,
        const std::filesystem::path& output_dir,
        const wt::sensor::sensor_t& sensor,
        const std::string& sensor_name,
        const std::string& scene_name,
        const wt::scene::developed_polarimetric_film_pair_t<1>& films,
        std::size_t spe_written) {
    // TODO
    throw std::runtime_error("not implemented!");
}
// 3D polarimetric films
inline void write_out_films(
        const wt::wt_context_t &ctx,
        const std::filesystem::path& output_dir,
        const wt::sensor::sensor_t& sensor,
        const std::string& sensor_name,
        const std::string& scene_name,
        const wt::scene::developed_polarimetric_film_pair_t<3>& films,
        std::size_t spe_written) {
    // TODO
    throw std::runtime_error("not implemented!");
}

// 2D scalar films
inline void write_out_films(
        const wt::wt_context_t &ctx,
        const std::filesystem::path& output_dir,
        const wt::sensor::sensor_t& sensor,
        const std::string& sensor_name,
        const std::string& scene_name,
        const wt::scene::developed_scalar_film_pair_t<2>& films,
        std::size_t spe_written) {
    write_out_2d(ctx, output_dir, sensor, sensor_name, scene_name,
                 *films.developed, spe_written,
                 wt::bitmap::colour_encoding_type_e::linear,
                 false);
    // if we also have tonemapped results, write these out as well
    if (films.developed_tonemapped) {
        write_out_2d(ctx, output_dir, sensor, sensor_name + "_tonemapped",
                     scene_name, *films.developed_tonemapped, spe_written,
                     films.tonemapped_film_colour_encoding,
                     true);
    }
}

// 2D polarimetric films
inline void write_out_films(
        const wt::wt_context_t &ctx,
        const std::filesystem::path& output_dir,
        const wt::sensor::sensor_t& sensor,
        const std::string& sensor_name,
        const std::string& scene_name,
        const wt::scene::developed_polarimetric_film_pair_t<2>& films,
        std::size_t spe_written) {
    // write each component of the developed Stokes parameters films
    write_out_2d(ctx, output_dir, sensor, sensor_name + "_I", scene_name,
                 (*films.developed)[0], spe_written,
                 wt::bitmap::colour_encoding_type_e::linear,
                 false);
    write_out_2d(ctx, output_dir, sensor, sensor_name + "_Q", scene_name,
                 (*films.developed)[1], spe_written,
                 wt::bitmap::colour_encoding_type_e::linear,
                 false);
    write_out_2d(ctx, output_dir, sensor, sensor_name + "_U", scene_name,
                 (*films.developed)[2], spe_written,
                 wt::bitmap::colour_encoding_type_e::linear,
                 false);
    write_out_2d(ctx, output_dir, sensor, sensor_name + "_V", scene_name,
                 (*films.developed)[3], spe_written,
                 wt::bitmap::colour_encoding_type_e::linear,
                 false);

    // if we also have tonemapped results, write these out as well
    if (films.developed_tonemapped) {
        write_out_2d(ctx, output_dir, sensor, sensor_name + "_tonemapped_I", scene_name,
                     (*films.developed_tonemapped)[0], spe_written,
                     films.tonemapped_film_colour_encoding,
                     true);
        write_out_2d(ctx, output_dir, sensor, sensor_name + "_tonemapped_Q", scene_name,
                     (*films.developed_tonemapped)[1], spe_written,
                     films.tonemapped_film_colour_encoding,
                     true);
        write_out_2d(ctx, output_dir, sensor, sensor_name + "_tonemapped_U", scene_name,
                     (*films.developed_tonemapped)[2], spe_written,
                     films.tonemapped_film_colour_encoding,
                     true);
        write_out_2d(ctx, output_dir, sensor, sensor_name + "_tonemapped_V", scene_name,
                     (*films.developed_tonemapped)[3], spe_written,
                     films.tonemapped_film_colour_encoding,
                     true);
    }
}

void write_render_result(const wt::wt_context_t &ctx,
                         const wt::scene_t &scene,
                         const wt::ads::ads_t &ads,
                         const wt::scene::render_result_t& render_result,
                         const bool intermediate) {
    auto output_dir = ctx.output_path;
    const auto& scene_name  = scene.get_id();

    if (intermediate) {
        // store in subdirs
        const auto name = std::format("intermediate_{:%F_%H-%M-%S}", std::chrono::system_clock::now());
        output_dir = output_dir / name;
    }
    // create output dir
    if (!std::filesystem::exists(output_dir))
        std::filesystem::create_directories(output_dir);

    // watermark and write out
    for (const auto& f : render_result.sensors) {
        // write out developed films
        std::visit([&](const auto& films) {
            write_out_films(ctx, output_dir, *f.second.sensor, f.first, scene_name, films, f.second.spe_written);
        }, f.second.developed_films);
    }

    // dump performance stats as well
    if (should_write_stats_to_stdout_on_exit) {
        wt::logger::cout() << "Writing performance stats to file..." << '\n';
        print_stats_to_file(output_dir / "perf_stats.csv", render_result);
    }
}


/* Rendering progress
 */

struct renderer_progressbar_t {
    static constexpr auto pb_name   = "__scene_renderer";
    static constexpr auto pb_colour = wt::logger::colour::yellow;
    static constexpr auto pb_len    = 20ul;

    renderer_progressbar_t(const std::string& sensor_id) : sensor_pb_name(pb_name + sensor_id) {
        auto sensor_name = sensor_id;
        if (sensor_name.length()>pb_len)
            sensor_name.resize(pb_len);
        while (sensor_name.length()<pb_len) sensor_name+=" ";

        wt::logger::cout.add_progress_bar(sensor_pb_name, pb_colour);
        wt::logger::cout.pb(sensor_pb_name).set_prefix(std::move(sensor_name));
        wt::logger::cout.pb(sensor_pb_name).set_progress(.0f);
    }

    void set_progress(wt::f_t p) noexcept {
        wt::logger::cout.pb(sensor_pb_name).set_progress(p);
    }

    void mark_terminated() noexcept {
        wt::logger::cout.pb(sensor_pb_name).set_foreground_colour(wt::logger::colour::red);
        wt::logger::cout.pb(sensor_pb_name).set_postfix("<aborted>");
        wt::logger::cout.pb(sensor_pb_name).set_show_elapsed_time(true);
        wt::logger::cout.pb(sensor_pb_name).set_show_remaining_time(true);
        wt::logger::cout.pb(sensor_pb_name).detach();
    }
    template <typename Rep, typename Period>
    void mark_completed(std::chrono::duration<Rep, Period> elapsed_time) noexcept {
        const auto& [elapsed_days,elapsed_hours] = wt::format::chrono::extract_duration<std::chrono::days>(elapsed_time);
        const auto& elapsed = 
            (elapsed_days>0 ? std::format("{} days ", elapsed_days) : "") + 
            std::format("{:%Hh:%Mm:%Ss}",floor<std::chrono::seconds>(elapsed_hours));
        
        const std::string str = "[" + elapsed + "] ✓";

        wt::logger::cout.pb(sensor_pb_name).set_foreground_colour(wt::logger::colour::green);
        wt::logger::cout.pb(sensor_pb_name).set_postfix(str);
        wt::logger::cout.pb(sensor_pb_name).set_show_elapsed_time(false);
        wt::logger::cout.pb(sensor_pb_name).set_show_remaining_time(false);
        wt::logger::cout.pb(sensor_pb_name).set_progress(1);
        wt::logger::cout.pb(sensor_pb_name).complete();
    }

    const std::string sensor_pb_name;
};


/* Rendering
 */

using wt_scene_defines_t = wt::scene::loader::defaults_defines_t;
using scene_bootstrap_t =
    wt::scene::scene_bootstrap_t<
        wt::scene::loader::xml::xml_loader_t,
        wt::ads::construction::bvh8w_constructor_t
    >;

// checks args, updates wt context with args, and prepares global resources, like the threadpool.
void initialize_renderer(
        const std::filesystem::path& scene_path,
        const std::optional<std::filesystem::path>& output_dir_path,
        const std::optional<std::filesystem::path>& scene_data_path,
        const std::optional<std::uint32_t>& cpu_threadpool_size) {
    // data path
    context.scene_data_path = !scene_data_path ? scene_path.parent_path() : scene_data_path.value();
    // output path defaults to scene directory
    context.output_path = output_dir_path.value_or(scene_path.parent_path());


    // create output dir
    if (!std::filesystem::exists(context.output_path)) {
        if (!std::filesystem::create_directories(context.output_path))
            throw std::runtime_error("Output directory \"" + context.output_path.string() + "\" could not be created");
    }

    if (!is_path_file_readable(scene_path))
        throw std::runtime_error("Scene data path \"" + scene_path.string() + "\" is not readable");
    else if (!is_path_dir_writeable(context.output_path))
        throw std::runtime_error("Output path \"" + context.output_path.string() + "\" is not a writeable directory");
    else if (!is_path_dir_readable(context.scene_data_path))
        throw std::runtime_error("Can't read from work path");

    if (context.renderer_block_size==0 || context.renderer_samples_per_block==0)
        throw std::runtime_error("Render block size and samples per block must be positive");


    // initialize thread pool
    threadpool = !cpu_threadpool_size ?
        std::make_unique<wt::thread_pool::tpool_t>() :
        std::make_unique<wt::thread_pool::tpool_t>(cpu_threadpool_size.value());
    context.threadpool = threadpool.get();


#ifdef DEBUG_FP_EXCEPTIONS
    // fp exceptions
    feenableexcept(FE_INVALID | FE_OVERFLOW);
#endif
}

inline void render(
        const std::filesystem::path& scene_path,
        const wt_scene_defines_t& scene_loader_defines,
        const std::string& preview_tev_host_port_str) {
    // load scene and construct ADS
    {
        // scene name: parent directory + scene file name
        auto scene_name = 
            (scene_path.has_parent_path() ?
                scene_path.parent_path().filename()/scene_path.filename() :
                scene_path.filename()
            ).string();

        wt::logger::cout(wt::verbosity_e::normal) << "loading scene \'" << scene_name << "\'..." << '\n';
        
        // progress bars
        static const std::string pb_name_scene = "__scene_loader";
        static const std::string pb_name_shared_res = "__res_loader";
        static const std::string pb_name_ads = "__ads_ctor";
        wt::logger::cout.add_progress_bar(pb_name_scene, wt::logger::colour::cyan);
        wt::logger::cout.pb(pb_name_scene).set_prefix("Loading scene       ");
        wt::logger::cout.pb(pb_name_scene).set_progress(0);
        wt::logger::cout.add_progress_bar(pb_name_shared_res, wt::logger::colour::green);
        wt::logger::cout.pb(pb_name_shared_res).set_prefix("Loading resources   ");
        wt::logger::cout.pb(pb_name_shared_res).set_progress(0);
        wt::logger::cout.add_progress_bar(pb_name_ads, wt::logger::colour::blue);
        wt::logger::cout.pb(pb_name_ads).set_prefix("Constructing ADS    ");
        wt::logger::cout.pb(pb_name_ads).set_progress(0);

        auto pb_prog = wt::scene::bootstrap_progress_callback_t{
            .scene_loading_progress_update = [](wt::f_t p) noexcept {
                wt::logger::cout.pb(pb_name_scene).set_progress(p);
            },
            .resources_loading_progress_update = [](wt::f_t p) noexcept {
                wt::logger::cout.pb(pb_name_shared_res).set_progress(p);
            },
            .ads_progress_update = [](wt::f_t p) noexcept {
                wt::logger::cout.pb(pb_name_ads).set_progress(p);
            },
            .on_finish = []() noexcept {
                wt::logger::cout.pb(pb_name_scene).complete();
                wt::logger::cout.pb(pb_name_shared_res).complete();
                wt::logger::cout.pb(pb_name_ads).complete();
            },
        };

        auto bootstrapper = std::make_unique<scene_bootstrap_t>(
            std::move(scene_name),
            scene_path,
            context,
            scene_loader_defines,
            std::move(pb_prog)
        );

        // wait for loader and check for errors
        bootstrapper->wait();
        if (bootstrapper->get_scene_loader()->has_errors())
            throw std::runtime_error("Bootstrap failed");

        scene = std::move(*bootstrapper).get_scene();
        ads   = std::move(*bootstrapper).get_ads();

        wt::logger::cout.end_progress_bars_group();
    }

    print_summary();


#ifndef WIN32
    {
        struct sigaction sa;
        sa.sa_handler = sigint_handler;
        sigaction(SIGINT, &sa, nullptr);
    }
#endif
    

    wt::scene::render_opts_t render_opts = {};

    // use preview?
    std::unique_ptr<wt::preview_tev_t> previewer;
    if (!preview_tev_host_port_str.empty()) {
        const auto host_port = wt::parse_hostname_and_port(preview_tev_host_port_str);
        // connect to a tev instance
        previewer = std::make_unique<wt::preview_tev_t>(host_port.first, host_port.second);
        render_opts.previewer = previewer.get();
    }

    // progress bars
    // TODO: multi sensor progress bars?
    assert(scene->sensors().size()==1);
    renderer_progressbar_t pb{ scene->sensors().begin()->get_sensor()->get_id() };
    render_opts.progress_callback = wt::scene::render_opts_t::progress_callback_t{
        .progress_update = [&](const auto& sensor_id, auto p) noexcept { pb.set_progress(p); },
        .on_complete = [&](const auto& sensor_id, const auto& elapsed_time) noexcept { pb.mark_completed(elapsed_time); },
        .on_terminate = [&](const auto& sensor_id) noexcept { pb.mark_terminated(); },
    };

    // start rendering
    scene_renderer = std::make_unique<wt::scene_renderer_t>(
            *scene, context, *ads, std::launch::async, std::move(render_opts));

    // wait for render process to complete and retrieve results
    const auto& render_result = scene_renderer->get();
    // write out
    write_render_result(context, *scene, *ads, render_result, false);


    // write out additional performance stats
    if (should_print_stats_to_stdout_on_exit)
        print_stats_to_stdout();
}

inline void render_gui(
        const std::filesystem::path& scene_path,
        const wt_scene_defines_t& scene_loader_defines) {
    // start GUI
    // this takes over the loading and rendering process
    auto gui = wt::gui_t::launch<scene_bootstrap_t>(
            context, scene_path, scene_loader_defines,
            [](const auto& scene, const auto& ads, const auto& render_results, bool intermediate) {
                write_render_result(context, scene, ads, render_results, intermediate);
            });

    // write out additional performance stats
    if (should_print_stats_to_stdout_on_exit)
        print_stats_to_stdout();
}


/* CLI parsing helpers
 */

using defines_t = std::vector<std::string>;

inline auto parse_defines(defines_t defs) {
    wt_scene_defines_t scene_defines;

    for (const auto& d : defs) {
        std::stringstream ss(d); 

        std::string def;
        if (!std::getline(ss, def, '='))
            throw std::runtime_error("Malformed define \"" + d + "\"");

        std::string val;
        if (ss.peek()=='\'') {
            ss.ignore();
            std::getline(ss, val, '\'');
            while (!ss.eof() && (ss.peek()==' ' || ss.peek()=='\t')) ss.ignore();
            if (!ss.eof())
                throw std::runtime_error("Malformed define \"" + def + "\"");
            ss.ignore();
        } else {
            std::getline(ss, val, '\0');
        }

        const auto& k = wt::format::trim(def);
        if (scene_defines.contains(k))
            throw std::runtime_error("Duplicate define \"" + k + "\"");
        scene_defines[k] = wt::format::trim(val);
    }

    defs = {};
    return scene_defines;
}


/* Main
 */

int main(int argc, char* argv[]) {
    // set system locale globally
    std::locale::global(std::locale(""));


    // enable OpenEXR's internal multithreading
    // (this is used in addition to our thread pool, so do not use many threads)
    Imf::setGlobalThreadCount(4);


    /*
     * Configure CLI and parse
    */

    std::filesystem::path scene_path;
    std::optional<std::filesystem::path> output_dir_path;
    std::optional<std::filesystem::path> scene_data_path;

    std::optional<std::uint32_t> cpu_threadpool_size;

    defines_t defines;

    wt::logger::log_verbosity_e filelog_verbosity = wt::logger::log_verbosity_e::info;

    bool no_progress_bars = false;
    
    CLI::App cli{ "wave_tracer" };
    argv = cli.ensure_utf8(argv);

    cli.require_subcommand();


    /* "about" cli subcommand
     */

    auto &cli_version = *cli.add_subcommand("version", "Print version")
        ->ignore_case("true");

    cli_version.callback([&]() {
        wt::wt_version_t{}.print_wt_version();
    });


    /* common option group for all render subcommands
     */

    auto render_opt = std::make_shared<CLI::App>();

    render_opt->add_option("scene_file", scene_path,
                           "scene file to render")
        ->required()
        ->option_text("PATH")
        ->check(CLI::ExistingFile);
    render_opt->add_option("-o,--output", output_dir_path,
                           "rendered results output directory")
        ->option_text("PATH");
    render_opt->add_option("--scenedir", scene_data_path,
                           "path for scene resources loading (defaults to scene file directory)")
        ->option_text("PATH")
        ->check(CLI::ExistingDirectory);

    render_opt->add_option("-p,--threads", cpu_threadpool_size,
                           "number of parallel threads to use (defaults to hardware concurrency)");
    render_opt->add_option("-D,--define", defines,
                           "define variables (used as \"$variable\" in the scene file)")
        ->option_text("<NAME=VALUE>")
        ->delimiter(',')
        ->take_all();
    render_opt->add_flag("--ray-tracing", context.renderer_force_ray_tracing,
                         "forces ray tracing only");
    render_opt->add_option_function<std::string>("--mesh_scale",
            [](const auto& v){
                context.default_scale_for_imported_mesh_positions = wt::stoq_strict<wt::length_t>(v);
            },
            "default scale to apply to imported positions of external meshes; can be overridden per shape in the scene file.")
        ->default_val(std::format("{}", context.default_scale_for_imported_mesh_positions));

    // rendering fine tuning
    render_opt->add_option("--block_size", context.renderer_block_size,
                           "dimension size of a rendered image block")
        ->capture_default_str()
        ->group("renderer fine tuning");
    render_opt->add_option("--block_samples", context.renderer_samples_per_block,
                           "number of samples-per-pixel for a single rendered image block")
        ->capture_default_str()
        ->group("renderer fine tuning");

    // run-time performance statistics
    render_opt->add_flag("--print-stats,!--no-print-stats", should_print_stats_to_stdout_on_exit,
                         "toggles printing performance statistics to stdout on exit, defaults to TRUE unless --quiet is set")
        ->group("run-time performance statistics");
    render_opt->add_flag("--write-stats,!--no-write-stats", should_write_stats_to_stdout_on_exit,
                         "write performance statistics to a CSV file on exit")
        ->group("run-time performance statistics");

    // logging
    render_opt->add_flag("--filelog,!--no-filelog", use_file_logging,
                         "toggles logging to a file")
        ->group("logging");
    render_opt->add_option_function<std::string>("--filelog_verbosity",
            [&](const auto& v) {
                const auto val = wt::format::parse_enum<wt::verbosity_e>(v);
                if (!val)
                    throw CLI::ParseError("log_level parsing failed", CLI::ExitCodes::ConversionError);
                filelog_verbosity = *val;
            },
            "sets verbosity level of file logging")
        ->option_text("<quiet/important/normal/info/debug>")
        ->default_val("info")
        ->group("logging");

    // misc
    render_opt->add_flag("--watermark,!--no-watermark", watermark_results,
                         "disables watermarking the rendered output image")
        ->capture_default_str()
        ->group("misc");


    /* "render" cli subcommand
     */

    auto &cli_render = *cli.add_subcommand("render", "Render a scene")
        ->ignore_case("true");
    cli_render.add_subcommand(render_opt);
    CLI::TriggerOff(&cli_render, &cli_version);

    auto sout_verbosity = wt::logger::log_verbosity_e::normal;

    // verbosity
    auto opt_q = 
        cli_render.add_flag_callback("-q,--quiet",
            [&]() {
                sout_verbosity = wt::verbosity_e::quiet;
                should_print_stats_to_stdout_on_exit = false;
            },
            "suppresses debug/info output (log level = quiet)")
        ->group("verbosity");
    auto opt_vv = 
        cli_render.add_flag_callback("-v,--verbose",
            [&]() {
                sout_verbosity = wt::verbosity_e::info;
            },
            "additional informational output (log level = info)")
        ->group("verbosity");
    auto opt_cout_level = 
        cli_render.add_option_function<std::string>("--verbosity",
            [&](const auto& v) {
                const auto val = wt::format::parse_enum<wt::verbosity_e>(v);
                if (!val)
                    throw CLI::ParseError("verbosity parsing failed", CLI::ExitCodes::ConversionError);
                sout_verbosity = *val;
            },
            "sets verbosity level of standard output logging")
        ->option_text("<quiet/important/normal/info/debug>")
        ->default_val("normal")
        ->group("verbosity");
    cli_render.add_flag("--no-progress", no_progress_bars,
                        "suppresses progress bars")
        ->capture_default_str()
        ->group("verbosity");

    opt_q->excludes(opt_vv);
    opt_cout_level->excludes(opt_q);
    opt_cout_level->excludes(opt_vv);

    // preview
    const auto default_tev_host_port = std::string{ "127.0.0.1:14158" };
    std::string preview_tev_host_port_str;
    cli_render.add_flag("--tev{" + default_tev_host_port + "}", preview_tev_host_port_str,
                        "connect to a tev instance to display rendering preview (hostname:port)")
        ->group("preview interface");
    
    cli_render.callback([&]() {
        initialize_renderer(scene_path, output_dir_path, scene_data_path,
                cpu_threadpool_size);

        wt::logger::cout.set_sout_level(sout_verbosity);
        wt::logger::cwarn.set_sout_level(sout_verbosity);
        wt::logger::cerr.set_sout_level(sout_verbosity);
        initialize_logs(filelog_verbosity, no_progress_bars);
        
        // print version
        wt::wt_version_t{}.print_wt_version();

        // render
        const auto scene_loader_defines = parse_defines(std::move(defines));
        render(scene_path,
               scene_loader_defines,
               preview_tev_host_port_str);
    });


#ifdef GUI
    /* "renderui" cli subcommand
     */

    auto &cli_renderui = *cli.add_subcommand("renderui", "Render a scene with a GUI")
        ->ignore_case("true");
    cli_renderui.add_subcommand(render_opt);

    CLI::TriggerOff(&cli_renderui, &cli_version);
    CLI::TriggerOff(&cli_renderui, &cli_render);

    cli_renderui.callback([&]() {
        initialize_renderer(scene_path, output_dir_path, scene_data_path,
                    cpu_threadpool_size);
        // disable progress bars. GUI provides its own bars.
        initialize_logs(filelog_verbosity, true);
        
        // render
        const auto scene_loader_defines = parse_defines(std::move(defines));
        render_gui(scene_path,
                   scene_loader_defines);
    });
#endif


    // parse cli
    try {
        cli.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return cli.exit(e);
    }


    return terminate_program ? -1 : 0;
}
