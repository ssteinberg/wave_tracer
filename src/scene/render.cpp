/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <vector>

#include <future>

#include <format>
#include <cassert>
#include <chrono>

#include <wt/scene/scene_renderer.hpp>
#include <wt/scene/scene.hpp>
#include <wt/util/thread_pool/tpool.hpp>

#include <wt/util/logger/logger.hpp>

#include <wt/scene/scene_previewer.hpp>

using namespace wt;
using namespace wt::scene;
using namespace wt::scene::interrupts;


void print_sensor_summary(const sensor::sensor_t* sensor, const sensor::film_storage_handle_t* film) noexcept {
    const auto samples_per_element = sensor->requested_samples_per_element();
    const auto krange = sensor->sensitivity_spectrum().wavenumber_range();

    // print wavelength in µm for short wavelengths, otherwise print frequency
    const auto kmean = krange.centre();
    const auto format_as_freq = wavenum_to_wavelen(kmean)>=1*u::µm;
    const auto format_wl = [&](const auto& k) noexcept {
        if (format_as_freq) {
            const auto f = wavenum_to_freq(k);
            if (f >= 1*u::GHz)
                return std::format("{::N[.2f]}", f.in(u::GHz));
            return std::format("{::N[e]}", f.in(u::MHz));
        }
        if (wavenum_to_wavelen(krange.min)>=10*u::nm)
            return std::format("{::N[.0f]}", wavenum_to_wavelen(k).in(u::nm));
        return std::format("{::N[e]}", wavenum_to_wavelen(k).in(u::nm));
    };
    std::string wavelen_desc = krange.length()>zero ?
        std::format("{} — {}", 
            format_wl(format_as_freq ? krange.min : krange.max),
            format_wl(format_as_freq ? krange.max : krange.min)) :
        format_wl(kmean);

    // sensor description
    std::string snsr_desc = std::format("{:d}", film->film_size()[0]);
    for (std::size_t i=1;i<film->dimensions_count();++i)
        snsr_desc += "×" + std::format("{:d}", film->film_size()[i]);
    snsr_desc += " px @ " + std::format("{} spe", samples_per_element);

    using namespace logger::termcolour;

    auto cout = wt::logger::cout();
    cout
        << bold
        << std::format("sensor {:20}", "<"+sensor->get_id()+">")
        << reset
        << std::format(" {:30}  ", snsr_desc)
        << reset << bright_blue
        << (krange.length()>zero ? "[ " : "")
        << bold << wavelen_desc
        << reset << bright_blue
        << (krange.length()>zero ? " ]" : "");

    // flags
    if (sensor->is_polarimetric())
        cout
            << "  " << bold << bright_magenta << "Stokes";
    if (sensor->ray_trace_only()) 
        cout
            << "  " << bold << bright_red << "RT";

    cout
        << reset
        << '\n' << '\n';
}


struct alignas(64) task_completion_signal_t {
    std::condition_variable signal;
    std::mutex m;
} task_completion_signal;

struct completed_render_job_t {
    sensor::block_handle_t block;
};

struct block_renderer_t {
    auto operator()(const integrator::integrator_context_t& ctx,
                    const sensor::sensor_t* sensor,
                    const sensor::block_handle_t& block,
                    const std::size_t samples_per_block,
                    const std::size_t total_samples) {
        for_range(vec3u32_t{ 0 }, block.size, [&](auto pos_in_block) {
            // integrate
            ctx.scene->integrator().integrate(ctx,
                                              block, 
                                              pos_in_block+block.position,
                                              (std::uint32_t)samples_per_block);
        });
    }
};

struct render_context_t {
    const sensor::sensor_t* sensor;

    std::unique_ptr<sensor::film_storage_handle_t> film_storage;
    integrator::integrator_context_t integrator_ctx;

    const std::size_t total_jobs;
    std::size_t enqueued_jobs = 0, jobs_completed = 0;

    const f_t recp_total_jobs;

    const std::size_t samples_per_block;
    const std::size_t samples_per_element;

    task_completion_signal_t& task_completion_signal;
    std::vector<std::future<completed_render_job_t>> futures;

    render_context_t(const wt_context_t& ctx, const ads::ads_t& ads, const scene_t* scene,
                     task_completion_signal_t& task_completion_signal,
                     const sensor::sensor_t* sensor,
                     std::size_t total_jobs, std::size_t samples_per_block, std::size_t samples_per_element,
                     std::unique_ptr<sensor::film_storage_handle_t> film_storage) noexcept
        : sensor(sensor),
          film_storage(std::move(film_storage)),
          integrator_ctx(&ctx, scene, &ads, sensor, this->film_storage.get()),
          total_jobs(total_jobs),
          recp_total_jobs(f_t(1)/total_jobs),
          samples_per_block(samples_per_block),
          samples_per_element(samples_per_element),
          task_completion_signal(task_completion_signal)
    {
        assert(total_jobs % sensor->total_sensor_blocks() == 0);
    }

    inline void enqueue_next() noexcept {
        const auto blocks = sensor->total_sensor_blocks();
        const auto enqueued_samples = enqueued_jobs / blocks * samples_per_block;
        const auto spb = m::min(samples_per_block, samples_per_element-enqueued_samples);

        // acquire a block
        auto block = sensor->acquire_sensor_block(film_storage.get(), enqueued_jobs % blocks);
        // queue render job
        auto f =
            integrator_ctx.wtcontext->threadpool->enqueue([this, spb,
                                                           block=std::move(block)]() mutable {
                block_renderer_t{}(integrator_ctx, sensor, block, 
                                   spb, 
                                   samples_per_element);
                auto ret = completed_render_job_t{ .block=std::move(block) };

                // notify
                task_completion_signal.signal.notify_one();

                return ret;
            });
        futures.emplace_back(std::move(f));
        ++enqueued_jobs;
    }

    /**
     * @brief Attempts to enqueue jobs, up to 'jobs' count. Returns number of successfully enqueued jobs, or 0 if no jobs left to enqueue.
     */
    std::size_t enqueue_jobs(std::size_t jobs) noexcept {
        assert(enqueued_jobs<=total_jobs);

        auto j=0ul;
        for (;j<jobs;++j) {
            if (enqueued_jobs == total_jobs) break;
            enqueue_next();
        }

        return j;
    }

    /**
     * @brief Attempts to enqueue jobs, such that all image blocks receive the same samples-per-element count. Returns number of successfully enqueued jobs, or 0 if no jobs left to enqueue.
     */
    std::size_t enqueue_jobs_for_intermediate_render() noexcept {
        assert(enqueued_jobs<=total_jobs);

        const auto blocks = sensor->total_sensor_blocks();

        auto j=0ul;
        for (;(enqueued_jobs % blocks)!=0;++j) {
            if (enqueued_jobs == total_jobs) break;
            enqueue_next();
        }

        return j;
    }

    // Check if any jobs are done and completes them. Avoids blocking.
    inline std::size_t complete_jobs() noexcept {
        using namespace std::chrono_literals;

        if (futures.empty()) return 0;

        std::size_t done_count=0;
        for (auto it=futures.begin(); it!=futures.end();) {
            if (it->wait_for(0ms) == std::future_status::ready) {
                complete_job(std::move(*it).get());
                ++done_count;

                it = futures.erase(it);
            } else
                ++it;
        }

        jobs_completed += done_count;
        return done_count;
    }
    // Blocks and completes for all remaining jobs.
    inline void wait_and_complete_jobs() noexcept {
        for (auto& future : futures)
            complete_job(std::move(future).get());
        futures.clear();
    }

    [[nodiscard]] inline bool is_complete() const noexcept { return jobs_completed==total_jobs; }

    [[nodiscard]] f_t progress() const noexcept {
        return f_t(jobs_completed)*recp_total_jobs;
    }
    [[nodiscard]] f_t fractional_spp_complete() const noexcept {
        return progress()*f_t(samples_per_element);
    }
    [[nodiscard]] auto spp_complete() const noexcept {
        return (std::size_t)(m::round(fractional_spp_complete()) + f_t(.5));
    }

    [[nodiscard]] auto develop(const duration_t& render_elapsed_time) const {
        const auto spe_completed = this->spp_complete();

        sensor_render_result_t::developed_films_t developed_films;
        const auto tonemapped_film_colour_encoding = film_storage->get_colour_encoding_of_developed_tonemapped_film();

        if (film_storage->is_polarimetric()) {
            // develop polarimetric films - 4 component Stokes parameters per channel as pixel type
            developed_films = developed_polarimetric_film_pair_t{
                .developed_tonemapped = film_storage->get_tonemap() ?
                    std::make_unique<sensor::developed_polarimetric_film_t<2>>(
                        film_storage->develop_stokes_d2(spe_completed)) :
                    nullptr,
                .tonemapped_film_colour_encoding = tonemapped_film_colour_encoding,
                .developed = std::make_unique<sensor::developed_polarimetric_film_t<2>>(
                        film_storage->develop_lin_stokes_d2(spe_completed)),
            };
        } else {
            // develop scalar films - single fp per channel as pixel type
            developed_films = developed_scalar_film_pair_t{
                .developed_tonemapped = film_storage->get_tonemap() ?
                    std::make_unique<sensor::developed_scalar_film_t<2>>(
                        film_storage->develop_d2(spe_completed)) :
                    nullptr,
                .tonemapped_film_colour_encoding = tonemapped_film_colour_encoding,
                .developed = 
                    std::make_unique<sensor::developed_scalar_film_t<2>>(
                        film_storage->develop_lin_d2(spe_completed)),
            };
        }

        logger::cout(verbosity_e::info)
            << "(scene_renderer) developed film for <" << sensor->get_id() << ">: "
            << (film_storage->is_polarimetric() ? "polarimetric (Stokes) " : "")
            << film_storage->film_size().x << "×" << film_storage->film_size().y << " @ " << spe_completed << "spp" << '\n';

        return std::make_pair(
            sensor->get_id(), 
            sensor_render_result_t{
                .sensor = sensor,
                .render_elapsed_time = render_elapsed_time,
                .developed_films = std::move(developed_films),
                .spe_written = spe_completed,
                .fractional_spe = is_complete() ? std::nullopt : std::optional<f_t>{ fractional_spp_complete() },
            }
        );
    }

private:
    inline void complete_job(completed_render_job_t&& job) const noexcept {
        film_storage->write_block(job.block);
        // and release block for reuse
        sensor->release_sensor_block(film_storage.get(), std::move(job.block));
    }
};


using render_contexts_t = std::vector<render_context_t>;
using render_contexts_ptrs_t = std::vector<render_context_t*>;


inline void scene_renderer_t::renderer_state_t::process_pending_interrupts(void* render_ctxs_ptr) noexcept {
    if (has_pending_capture_interrupts()) {
        auto& render_ctxs = *reinterpret_cast<render_contexts_t*>(render_ctxs_ptr);

        // intermediate results capture interrupts require paused state
        // process once all jobs have completed
        if (jobs_enqueued==0) {
            render_result_t results;
            results.render_elapsed_time = this->elapsed_time();

            for (auto& rctx : render_ctxs)
                results.sensors.emplace(rctx.develop(results.render_elapsed_time));

            for (const auto& intr : pending_capture_intermediate_interrupts)
                (*dynamic_cast<const capture_intermediate_t*>(intr.get()))(std::move(results));

            pending_capture_intermediate_interrupts.clear();
            paused = saved_paused_state;
        }
    }
}

void scene_renderer_t::process_interrupts(void* incomplete_render_ctxs_ptr) {
    const bool interrupt_signalled = interrupt_flag.test(std::memory_order_acquire);
    if (interrupt_signalled) {
        auto& incomplete_render_ctxs = *reinterpret_cast<render_contexts_ptrs_t*>(incomplete_render_ctxs_ptr);

        // process interrupts
        std::unique_lock l(*interrupts_queue_mutex);
        while (!interrupts_queue.empty()) {
            auto interrupt = std::move(interrupts_queue.back());
            interrupts_queue.pop();

            if (const auto pause = dynamic_cast<const pause_t*>(interrupt.get()); pause) {
                logger::cout(verbosity_e::info) << "(scene_renderer) pause interrupt." << '\n';
                state.paused = true;
            }
            else if (const auto resume = dynamic_cast<const resume_t*>(interrupt.get()); resume) {
                logger::cout(verbosity_e::info) << "(scene_renderer) resume interrupt." << '\n';
                state.paused = false;
            }
            else if (const auto terminate = dynamic_cast<const terminate_t*>(interrupt.get()); terminate) {
                logger::cout(verbosity_e::info) << "(scene_renderer) terminate interrupt." << '\n';
                state.terminated = true;
            }
            else if (const auto capture = dynamic_cast<const capture_intermediate_t*>(interrupt.get()); capture) {
                logger::cout(verbosity_e::info) << "(scene_renderer) capture intermediate interrupt." << '\n';

                state.pending_capture_intermediate_interrupts.emplace_back(std::move(interrupt));
                // queue all remaining blocks
                for (auto& rctx : incomplete_render_ctxs) {
                    const auto enqueued = rctx->enqueue_jobs_for_intermediate_render();
                    state.jobs_enqueued += enqueued;
                }
                // and pause
                state.saved_paused_state = state.paused;
                state.paused = true;
            }
        }

        interrupt_flag.clear(std::memory_order_relaxed);
    }
}


scene_renderer_t::scene_renderer_t(
        const scene_t& scene,
        const wt_context_t& ctx, const ads::ads_t& ads,
        std::launch launch_policy,
        scene::render_opts_t opts) {
    this->future = std::async(launch_policy, [&, opts=std::move(opts)]() mutable {
        return this->render(&scene, ctx, ads, std::move(opts));
    });
}

scene::render_result_t scene_renderer_t::render(
        const scene_t* scene,
        const wt_context_t& ctx, const ads::ads_t& ads,
        const scene::render_opts_t opts) {
    using clock = std::chrono::steady_clock;

    // grab a copy of sensors to render
    const auto& sensors = scene->sensors();

    task_completion_signal_t task_completion_signal;

    // build render ctxs
    std::size_t total_jobs = 0;
    render_contexts_t render_ctxs;
    for (const auto& scs : sensors) {
        const auto* s = scs.get_sensor();

        auto film_storage = s->create_sensor_film(ctx, scene->integrator().sensor_write_flags());
        const auto samples_per_element = s->requested_samples_per_element();
        // ignore 0spp sensors (useful to selectively turn off sensors)
        if (samples_per_element==0) continue;

        // Blocks
        const auto samples_per_block = ctx.renderer_samples_per_block;
        const auto total_blocks = s->total_sensor_blocks();
        const auto blocks_to_queue = (samples_per_element+samples_per_block-1)/samples_per_block;
        const auto sensor_total_jobs = std::size_t(total_blocks)*blocks_to_queue;

        print_sensor_summary(s, film_storage.get());

        render_ctxs.emplace_back(ctx, ads, scene,
                                 task_completion_signal,
                                 s, 
                                 sensor_total_jobs, samples_per_block, samples_per_element, 
                                 std::move(film_storage));
        // reserve enqueued jobs storage
        render_ctxs.back().futures.reserve((std::size_t)((parallel_jobs_factor+1) * (f_t)ctx.threadpool->thread_count()));

        total_jobs += sensor_total_jobs;
    }

    wt::logger::cout(verbosity_e::info) << "(scene_renderer) starting render..." << '\n';

    state = {
        .total_jobs = total_jobs,
        .start_time = clock::now(),
    };
    state.last_checkpoint = state.start_time;

    // Enqueue initial render jobs
    const auto parallel_jobs_to_enqueue = (std::size_t)(m::ceil(parallel_jobs_factor * (f_t)ctx.threadpool->thread_count()));
    while (state.jobs_enqueued < parallel_jobs_to_enqueue) {
        bool jobs_left_to_enqueue = false;
        for (auto& rctx : render_ctxs) {
            const auto enqueued = rctx.enqueue_jobs(parallel_jobs_to_enqueue);
            state.jobs_enqueued += enqueued;
            if (enqueued>0) jobs_left_to_enqueue = true;
        }
        if (!jobs_left_to_enqueue)
            break;
    }

    render_contexts_ptrs_t incomplete_render_ctxs;
    incomplete_render_ctxs.reserve(render_ctxs.size());
    for (auto& rctx : render_ctxs)
        incomplete_render_ctxs.emplace_back(&rctx);

    // Do we have a render preview?
    std::unique_ptr<scene_previewer_t> previewer;
    if (opts.previewer) {
        scene_previewer_t::sensors_map_t preview_sensors;
        for (auto& rctx : render_ctxs)
            preview_sensors.emplace(rctx.sensor->get_id(), rctx.film_storage.get());
        previewer = std::make_unique<scene_previewer_t>(opts, preview_sensors);

        // initial empty preview
        for (const auto& rctx : render_ctxs)
            previewer->preview(rctx.sensor->get_id(), 0);
    }

    // Process render jobs
    bool fully_paused = false;
    for (;;) {
        // wait for signal
        {
            using namespace std::chrono_literals;
            std::unique_lock l(task_completion_signal.m);
            task_completion_signal.signal.wait_for(l, state.paused ? 1ms : 50us);
        }

        bool completed_jobs = false;
        // check if any jobs are done, and finalize them.
        for (auto& rctx : incomplete_render_ctxs) {
            const auto done = rctx->complete_jobs();
            if (done>0) {
                // jobs processed
                state.jobs_enqueued -= done;
                state.jobs_completed += done;
                completed_jobs = true;

                // update progress
                if (opts.progress_callback)
                    opts.progress_callback->progress_update(rctx->sensor->get_id(), rctx->progress());
            }
        }
        // keep track of state on completed jobs
        if (completed_jobs)
            state.checkpoint();
        fully_paused = state.paused && state.jobs_enqueued==0;

        // process interrupts that were previously registered and are awaiting processing
        state.process_pending_interrupts(&render_ctxs);
        // only check and process additional interrupts if the pending ones are completed
        if (!state.has_pending_interrupts())
            process_interrupts(&incomplete_render_ctxs);

        // enqueue additional jobs if needed
        if (!state.terminated && !state.paused) {
            // are we resuming?
            if (fully_paused)
                state.checkpoint(true);
            // enqueue
            while (state.jobs_enqueued < parallel_jobs_to_enqueue) {
                bool jobs_left_to_enqueue = false;
                for (auto& rctx : incomplete_render_ctxs) {
                    const auto enqueued = rctx->enqueue_jobs(parallel_jobs_to_enqueue);
                    state.jobs_enqueued += enqueued;
                    if (enqueued>0) jobs_left_to_enqueue = true;
                }
                if (!jobs_left_to_enqueue)
                    break;
            }
        }

        if (state.terminated)
            break;
        if (!completed_jobs)
            continue;

        // remove completed
        std::erase_if(incomplete_render_ctxs, [](const auto* rctx) { return rctx->is_complete(); });
        // done?
        if (incomplete_render_ctxs.empty()) {
            state.completed = true;
            break;
        }

        // preview
        if (previewer) {
            for (const auto& rctx : incomplete_render_ctxs)
                previewer->preview(rctx->sensor->get_id(), rctx->fractional_spp_complete());
        }
    }

    // wait for any remaining jobs (e.g., on early termination)
    for (auto& rctx : incomplete_render_ctxs) {
        rctx->wait_and_complete_jobs();
        if (!rctx->is_complete()) {
            const auto& id = rctx->sensor->get_id();
            wt::logger::cout(verbosity_e::info) 
                << "(scene_renderer) sensor <" << id << "> has incomplete rendering." << '\n';
            if (opts.progress_callback)
                opts.progress_callback->on_terminate(id);
        }
    }

    if (state.terminated) {
        wt::logger::cout(verbosity_e::info) << "(scene_renderer) rendering terminated." << '\n';
    }
    if (state.completed) {
        wt::logger::cout(verbosity_e::info) << "(scene_renderer) rendering completed successfully." << '\n';
    }

    // update preview with final render
    if (previewer) {
        for (const auto& rctx : render_ctxs)
            previewer->preview(rctx.sensor->get_id(), rctx.fractional_spp_complete());
        previewer = {};
    }

    // write out films for completed renders
    render_result_t ret;
    ret.render_elapsed_time = state.elapsed_time();

    for (auto& rctx : render_ctxs) {
        if (!rctx.is_complete()) continue;
        const auto& id = rctx.sensor->get_id();

        if (opts.progress_callback)
            opts.progress_callback->on_complete(id, ret.render_elapsed_time);

        ret.sensors.emplace(rctx.develop(ret.render_elapsed_time));
        assert(ret.sensors[id].spe_written == rctx.samples_per_element);
    }

    wt::logger::cout(verbosity_e::info) << "(scene_renderer) done. Elapsed: " << std::format("{:%H:%M:%S}", ret.render_elapsed_time) << '\n';

    return ret;
}


scene::rendering_status_t scene_renderer_t::rendering_status() const noexcept {
    std::atomic_thread_fence(std::memory_order_acquire);

    using rendering_status_t = scene::rendering_status_t;
    using rendering_state_t = scene::rendering_state_t;
    
    rendering_status_t ret = {
        .start_time = state.start_time,
        .elapsed_rendering_time = state.elapsed_time(),
        .total_blocks = state.total_jobs,
        .completed_blocks = state.jobs_completed,
        .blocks_in_progress = state.jobs_enqueued,
    };

    ret.state = rendering_state_t::rendering;
    if (state.completed)
        ret.state = rendering_state_t::completed_successfully;
    else if (state.terminated)
        ret.state = rendering_state_t::terminated;
    else if (state.paused && state.jobs_enqueued==0)
        ret.state = rendering_state_t::paused;
    else if (state.paused && state.jobs_enqueued>0)
        ret.state = rendering_state_t::pausing;

    return ret;
}
