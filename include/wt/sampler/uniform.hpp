/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <random>

#include <string>

#include "sampler.hpp"

#include <wt/wt_context.hpp>
#include <wt/math/common.hpp>
#include <wt/util/seeded_mt19937_64.hpp>

namespace wt::sampler {

/**
 * @brief Simple uniform sampler. Uses an std::mt19937_64 Mersenne Twister engine.
 */
class uniform_t final : public sampler_t {
    static thread_local seeded_mt19937_64 rd;

public:
    uniform_t(std::string id="")
        : sampler_t(std::move(id))
    {}
    uniform_t(uniform_t&&) = default;

    [[nodiscard]] inline f_t r() noexcept override {
        return std::uniform_real_distribution<f_t>{}(rd.engine());
    }
    [[nodiscard]] inline vec2_t r2() noexcept override {
        auto d = std::uniform_real_distribution<f_t>{};
        return { d(rd.engine()), d(rd.engine()) };
    }
    [[nodiscard]] inline vec3_t r3() noexcept override {
        auto d = std::uniform_real_distribution<f_t>{};
        return { d(rd.engine()), d(rd.engine()), d(rd.engine()) };
    }
    [[nodiscard]] inline vec4_t r4() noexcept override {
        auto d = std::uniform_real_distribution<f_t>{};
        return { d(rd.engine()), d(rd.engine()), d(rd.engine()), d(rd.engine()) };
    }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::unique_ptr<uniform_t> load(
        std::string id,
        scene::loader::loader_t* loader,
        const scene::loader::node_t& node,
        const wt::wt_context_t &context);
};

}
