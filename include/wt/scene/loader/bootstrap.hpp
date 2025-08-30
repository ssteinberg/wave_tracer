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
#include <future>
#include <string>
#include <istream>
#include <fstream>
#include <optional>

#include <wt/wt_context.hpp>
#include <wt/ads/ads_constructor.hpp>
#include <wt/scene/loader/loader.hpp>
#include <wt/ads/util.hpp>

#include <wt/util/unique_function.hpp>

namespace wt::scene {

/**
 * @brief Callbacks that will be called with scene loading progress updates.
 */
struct bootstrap_progress_callback_t {
    /** @brief Progress of scene loading update callback.
     */
    unique_function<void(f_t) const noexcept> scene_loading_progress_update;
    /** @brief Progress of resources loading update callback.
     */
    unique_function<void(f_t) const noexcept> resources_loading_progress_update;

    /** @brief Progress of ADS construction update callback.
     */
    unique_function<void(f_t) const noexcept> ads_progress_update;
    /** @brief ADS construction callback: provides a description of latest construction status.
     */
    unique_function<void(std::string) const noexcept> ads_construction_status_update;

    /** @brief Called on loading successful completion.
     */
    unique_function<void() const noexcept> on_finish;
};

/**
 * @brief Helper that constructs a scene and its accelerating data structure (ADS).
 *        Generic interface.
 */
class scene_bootstrap_generic_t {
public:
    virtual ~scene_bootstrap_generic_t() = default;

    /**
     * @brief Blocks and waits for scene loading to complete, and returns the constructed scene object.
     */
    virtual std::unique_ptr<scene_t> get_scene() && = 0;
    /**
     * @brief Blocks and waits for ADS construction to complete, and returns the constructed ADS object.
     */
    virtual std::unique_ptr<wt::ads::ads_t> get_ads() && = 0;

    /**
     * @brief Returns the scene loader object (may be null).
     */
    [[nodiscard]] virtual const loader::loader_t* get_scene_loader() const noexcept = 0;

    /**
     * @brief Blocks and waits for scene loading and ADS construction to complete.
     */
    virtual void wait() const = 0;
};

/**
 * @brief Helper that constructs a scene and its accelerating data structure (ADS).
 * @tparam SceneLoader scene loader type.
 * @tparam ADSCtor ADS constructor type.
 */
template <
    std::derived_from<loader::loader_t> SceneLoader,
    std::derived_from<ads::construction::ads_constructor_t> ADSCtor
>
class scene_bootstrap_t final : public scene_bootstrap_generic_t {
private:
    std::unique_ptr<loader::loader_t> sloader;
    std::future<std::unique_ptr<ads::ads_t>> ads_future;

    std::optional<bootstrap_progress_callback_t> callbacks;

private:
    inline void create(std::string name,
                       std::istream& scene_source,
                       const wt::wt_context_t& ctx,
                       const loader::defaults_defines_t& defines) {
        std::optional<loader::progress_callback_t> scene_prg_tracker;
        std::optional<ads::construction::progress_callback_t> ads_prg_tracker;

        // create progress trackers
        if (this->callbacks) {
            // tracks finished calls (2 - both scene loader and ADS completed successfully)
            auto finished = std::make_shared<std::atomic<int>>();
            *finished = 0;

            scene_prg_tracker = loader::progress_callback_t{
                .scene_loading_progress_update = std::move(this->callbacks->scene_loading_progress_update),
                .resources_loading_progress_update = std::move(this->callbacks->resources_loading_progress_update),
                .on_terminate = []() noexcept {},
                .on_finish =
                    [this, finished]() noexcept {
                        if ((++*finished)==2)
                            this->callbacks->on_finish();
                    },
            };
            ads_prg_tracker = ads::construction::progress_callback_t{
                .progress_update = std::move(this->callbacks->ads_progress_update),
                .on_finish =
                    [this, finished]() noexcept {
                        if ((++*finished)==2)
                            this->callbacks->on_finish();
                    },
                .status_description_update = std::move(this->callbacks->ads_construction_status_update),
            };
        }

        // start scene loading
        sloader = std::make_unique<SceneLoader>(
                std::move(name),
                ctx,
                scene_source,
                defines,
                std::move(scene_prg_tracker));

        ads_future = std::async(std::launch::async, [this, &ctx, pt=std::move(ads_prg_tracker)]() mutable {
            // get shapes
            auto shapes = sloader->get_shapes();
            // start building ADS
            return ADSCtor(shapes, ctx, std::move(pt)).get();
        });
    }

public:
    scene_bootstrap_t(
            std::string name,
            std::istream& scene_source,
            const wt::wt_context_t& ctx,
            const loader::defaults_defines_t& defines = {},
            std::optional<bootstrap_progress_callback_t> callbacks = {})
        : callbacks(std::move(callbacks))
    {
        create(std::move(name), scene_source, ctx, defines);
    }
    scene_bootstrap_t(
            std::string name,
            const std::filesystem::path& scene_path,
            const wt::wt_context_t& ctx,
            const loader::defaults_defines_t& defines = {},
            std::optional<bootstrap_progress_callback_t> callbacks = {})
        : callbacks(std::move(callbacks))
    {
        // read from path
        auto f = std::ifstream(scene_path);
        if (!f)
            throw std::runtime_error("(scene loader) Could not load \"" + scene_path.string() + "\"");
        create(std::move(name), f, ctx, defines);
    }

    virtual ~scene_bootstrap_t() {
        wait();
    }

    scene_bootstrap_t(scene_bootstrap_t&&) = default;

    /**
     * @brief Blocks and waits for scene loading to complete, and returns the constructed scene object.
     */
    std::unique_ptr<scene_t> get_scene() && override {
        auto scene = sloader->get();
        sloader = {};
        return scene;
    }

    /**
     * @brief Blocks and waits for ADS construction to complete, and returns the constructed ADS object.
     */
    std::unique_ptr<ads::ads_t> get_ads() && override {
        return std::move(ads_future).get();
    }

    /**
     * @brief Returns the scene loader object.
     */
    [[nodiscard]] const loader::loader_t* get_scene_loader() const noexcept override { return sloader.get(); }

    /**
     * @brief Blocks and waits for scene loading and ADS construction to complete.
     */
    void wait() const override {
        if (ads_future.valid())
            ads_future.wait();
        if (sloader)
            sloader->wait();
    }
};

}
