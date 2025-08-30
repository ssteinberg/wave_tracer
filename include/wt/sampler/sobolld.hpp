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
#include <string>

#include <wt/scene/element/scene_element.hpp>
#include <wt/wt_context.hpp>
#include <wt/math/common.hpp>

#include "sampler.hpp"

namespace wt::sampler {

/**
 * @brief Low-discrepancy Sobol sequence sampler.
 *        From "Quad-Optimized Low-Discrepancy Sequences", Ostromoukhov et al. 2024
 *        https://github.com/liris-origami/Quad-Optimized-LDS
 */
class sobolld_t final : public sampler_t {
    struct sobolld_impl_t;

private:
    std::unique_ptr<sobolld_impl_t> pimpl;

    static thread_local std::vector<f_t> sobol_samples;
    static thread_local std::size_t next_point_idx;

    void generate_samples() const noexcept;
    [[nodiscard]] inline const auto* next_sample(int count) const noexcept {
        if (next_point_idx+count>=sobol_samples.size())
            generate_samples();

        const auto* ret = &sobol_samples[next_point_idx];
        next_point_idx+=count;

        return ret;
    }

public:
    sobolld_t(std::string id="");
    sobolld_t(sobolld_t&&) = default;
    virtual ~sobolld_t() noexcept;

    [[nodiscard]] inline f_t r() noexcept override {
        const auto* s = next_sample(1);
        return s[0];
    }
    [[nodiscard]] inline vec2_t r2() noexcept override {
        const auto* s = next_sample(2);
        return { s[0], s[1] };
    }
    [[nodiscard]] inline vec3_t r3() noexcept override {
        const auto* s = next_sample(3);
        return { s[0], s[1], s[2] };
    }
    [[nodiscard]] inline vec4_t r4() noexcept override {
        const auto* s = next_sample(4);
        return { s[0], s[1], s[2], s[3] };
    }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::shared_ptr<sobolld_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);

private:
    void deferred_load(const wt::wt_context_t &context);
};

}
