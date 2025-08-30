/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <bitset>

#include <wt/math/common.hpp>
#include <wt/math/frame.hpp>
#include <wt/sampler/density.hpp>

#include <wt/interaction/intersection.hpp>
#include <wt/interaction/polarimetric/stokes.hpp>
#include <wt/interaction/polarimetric/mueller.hpp>

namespace wt::bsdf {

/**
 * @brief Mode of light transport.
 */
enum class transport_e : std::uint8_t {
    /**
     * @brief emitter to sensor transport
     */
    forward  = 0,
    /**
     * @brief sensor to emitter transport
     */
    backward = 1,
};

[[nodiscard]] inline constexpr auto flip_transport(transport_e t) noexcept {
    return t==transport_e::forward ? 
        transport_e::backward : transport_e::forward;
}


using lobe_mask_t = std::bitset<32>;

/**
 * @brief Data for a BSDF evaluation.
 */
struct bsdf_query_t {
    const intersection_surface_t& intersection;

    // wavenumber
    wavenumber_t k;

    transport_e transport;
    lobe_mask_t lobe = lobe_mask_t{}.set();
};


/**
 * @brief The BSDF of a polarimetric light-matter interaction, quantified by a Mueller operator.
 */
struct bsdf_result_t {
    mueller_operator_t M;

    static constexpr bool polarimetric = true;

    [[nodiscard]] inline auto mean_intensity() const noexcept {
        return M.mean_intensity();
    }
};


/**
 * @brief Sample returned from a bsdf_t::sample() query.
 */
struct bsdf_sample_t {
    dir3_t wo;

    solid_angle_sampling_pd_t dpd;

    c_t eta = 1;
    
    lobe_mask_t lobe = lobe_mask_t{};

    /**
     * @brief bsdf / pdf
     */
    bsdf_result_t weighted_bsdf;
};

}
