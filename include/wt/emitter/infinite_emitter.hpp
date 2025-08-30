/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/emitter/emitter.hpp>

#include <wt/math/common.hpp>
#include <wt/math/shapes/aabb.hpp>
#include <wt/wt_context.hpp>

namespace wt::emitter {

/**
 * @brief Generic interface for infinite emitters.
 *        Sampling of infinite emitters is slightly different compared with regular emitters. During construction, `wt::scene_t` populates the scene-wide bounding AABB ``world_aabb`` of all infinite emitters.
 *        The infinite emitters' sampling API changes as follows:
 *        * `wt::emitter::emitter_t::sample_position()` and `wt::emitter::emitter_t::pdf_position()` are unused and return 0.
 *        * `wt::emitter::emitter_t::sample()` samples with respect to positions within the ``world_aabb``, and not with respect to positions upon the emitter.
 *        * An alternative density query `wt::emitter::infinite_emitter_t::pdf_target_position()` provides the area sampling density for sampled world positions.
 */
class infinite_emitter_t : public emitter_t {
    friend class wt::scene_t;

protected:
    aabb_t world_aabb = aabb_t::null();
    virtual void set_world_aabb(const aabb_t& waabb) noexcept {
        world_aabb = waabb;
    }

public:
    infinite_emitter_t(std::string id, 
                       f_t emitter_phase_space_extent_scale = 1)
        : emitter_t(std::move(id), emitter_phase_space_extent_scale)
    {}
    infinite_emitter_t(const infinite_emitter_t&) = default;
    infinite_emitter_t(infinite_emitter_t&&) = default;
    
    [[nodiscard]] inline bool is_infinite_emitter() const noexcept final { return true; }

    /**
     * @brief Sampling area PDF of a target scene position, sampled by the infinite emitter's `sample()`.
     * @param wp sampled scene position
     */
    [[nodiscard]] virtual area_sampling_pd_t pdf_target_position(const pqvec3_t& wp) const noexcept = 0;

    /**
     * @brief Samples an emission phase-space position (ray) on the light source.
     *        Not implemented for infinite emitters.
     */
    [[nodiscard]] position_sample_t sample_position(sampler::sampler_t& sampler) const noexcept final {
        // infinite emitter: well designed integrators should not arrive here
        assert(false);
        return {};
    }

    /**
     * @brief Sampling PDF of an emission position on the light source.
     *        Not implemented for infinite emitters.
     */
    [[nodiscard]] area_sampling_pd_t pdf_position(
            const pqvec3_t& p,
            const intersection_surface_t* surface=nullptr) const noexcept final {
        // infinite emitter: well designed integrators should not arrive here
        assert(false);
        return {};
    }
};

}

