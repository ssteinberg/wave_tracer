/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <vector>
#include <optional>

#include <wt/ads/ads.hpp>
#include <wt/ads/intersection_record.hpp>
#include <wt/sampler/sampler.hpp>

#include <wt/math/common.hpp>
#include <wt/sampler/density.hpp>

#include <wt/interaction/intersection.hpp>

#include <wt/interaction/fsd/common.hpp>

namespace wt {

/**
 * @brief Free-space diffraction angular scattering function.
 */
class free_space_diffraction_t {
private:
    utd::fsd_aperture_t aperture;
    pqvec3_t interaction_wp;

public:
    struct sample_ret_t {
        pqvec3_t diffraction_wp;
        dir3_t wo = { 0,0,1 };
        // edge's surface
        std::optional<intersection_edge_t> intersection;

        // sampled a direct (passthrough) term
        bool is_direct;

        angle_sampling_pd_t dpd = angle_sampling_pd_t::discrete(0);
        f_t weight;
    };

    struct diffracting_edge_t {
        utd::UTD_ret_t utd;
        ads::tuid_t edge_idx;
        pqvec3_t p;
        dir3_t wi,wo;
        length_t ri, ro;
    };
    using eval_ret_t = std::vector<diffracting_edge_t>;

public:
    /**
     * @brief Construct a new free space diffraction object
     * 
     * @param k wavenumber
     * @param edges edges accessor
     */
    free_space_diffraction_t(
            const ads::ads_t* ads,
            const pqvec3_t& interaction_wp,
            const frame_t&  interaction_region_frame,
            const pqvec3_t& interaction_region_size,
            const dir3_t& wi,
            wavenumber_t k,
            const ads::intersection_record_t::edges_container_t& edges) noexcept;
    free_space_diffraction_t(free_space_diffraction_t&&) noexcept = default;

    /**
     * @brief Returns TRUE if the aperture is empty
     */
    [[nodiscard]] bool empty() const noexcept { return aperture.edges.empty(); }

    /**
     * @brief Samples the free-space diffraction BSDF.
     *        Returned position and direction are in world coordinates.
     * @param src source point in world coordinates
     */
    [[nodiscard]] sample_ret_t sample(const ads::ads_t* ads,
                                      const pqvec3_t& src, sampler::sampler_t& sampler) const noexcept;

    /**
     * @brief Evaluates the free-space diffraction BSDF.
     * @param src source point in world coordinates
     * @param dst target point in world coordinates
     */
    [[nodiscard]] eval_ret_t f(const pqvec3_t& src, const pqvec3_t& dst) const noexcept;

    /**
     * @brief Queries the sampling density of the free-space diffraction BSDF.
     * @param src source point in world coordinates
     * @param wo  world direction from interation point
     */
    [[nodiscard]] angle_sampling_pd_t pdf(const pqvec3_t& src, const dir3_t& wo) const noexcept;
};

}
