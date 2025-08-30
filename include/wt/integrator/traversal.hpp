/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cassert>
#include <variant>

#include <wt/util/unreachable.hpp>

#include <wt/scene/shape.hpp>
#include <wt/ads/ads.hpp>
#include <wt/beam/beam_generic.hpp>

#include <wt/math/common.hpp>

namespace wt::integrator {

// for numerical reasons: scale ballistic propagation distance by a little, this way the ballistic and diffusive segments are guaranteed to overlap
static constexpr f_t ballistic_scale = 1.001;

inline length_t calculate_min_ballistic_distance(const elliptic_cone_t& envelope, const ray_t& ray) noexcept {
    if (ray.o!=envelope.o()) {
        const auto rl = envelope.frame().to_local(ray.o - envelope.o()) * vec3_t{ 1,envelope.get_e(),1 };

        const auto dist_to_ray_inclusion = (m::length(pqvec2_t{ rl.x,rl.y }) - envelope.x0()) / envelope.get_tan_alpha() - rl.z;
        return m::max<length_t>(0*u::m, -rl.z, dist_to_ray_inclusion);
    }

    return 0*u::m;
}

inline length_t max_ballistic_distance(
        const Wavelength auto lambda,
        const std::uint32_t segment,
        const length_t min_ballistic_distance) noexcept {
    // Do a ballistic segment for 16/64/256/... wavelengths
    static constexpr std::uint64_t max_ballistic_segments = 16;
    static constexpr std::uint64_t ballistic_segment_lambdas = 8;
    static constexpr std::uint64_t max_ballistic_segment_lambdas = 1<<16;

    // minimal distance is self intersection distance, scaled a little for numerics
    // TODO: do better
    static constexpr f_t scale_self_intrs_dist = 1.05;
    const auto min_dist = min_ballistic_distance * scale_self_intrs_dist;

    const auto B = m::min(max_ballistic_segment_lambdas, ballistic_segment_lambdas << (2*segment+1));
    return segment>=max_ballistic_segments ? 
            limits<length_t>::infinity() :
            length_t{ min_dist + lambda * f_t(B) };
}


struct traversal_opts_t {
    const bool force_ray_tracing = false;
    const bool detect_edges = true;
    const bool accumulate_edges = false;
};

struct traversal_result_t {
    /** @brief Origin of beam traversal. This is the real origin, and may be shifted for self-intersection avoidance.
     */
    pqvec3_t origin;

    /** @brief Intersection record
     */
    ads::intersection_record_t record;
    /** @brief z distance (starting from ``record.dist``) over which triangles are considered for intersection
     */
    length_t intersection_region_depth = 0 * u::m;

    /** @brief Was traversal ballistic?
     */
    bool ballistic = false;
};
struct shadow_result_t {
    bool shadow;
    // traversal was ballistic?
    bool ballistic = false;
};


/**
 * @brief Traverses the ADS with a cone.
 *        Starts with doing short ballistic (coherent photons) segments, propagated as rays,
 *        after each segment attempt to restart diffusive (beam) propagation.
 */
[[nodiscard]] inline traversal_result_t traverse(
        const ads::ads_t& ads,
        const elliptic_cone_t& envelope,
        const length_t lambda,
        const length_t distance,
        const traversal_opts_t& opts) noexcept {
    const auto& ray = envelope.ray();

    const bool ray_trace = opts.force_ray_tracing || envelope.is_ray();
    if (ray_trace) {
        // we are in pure ray tracing mode
        return { 
            .origin = ray.o,
            .record = ads.intersect(ray, { 0*u::m, distance }), 
            .ballistic = true
        };
    }

    // start with ballistic propagation
    // and try to resume diffusive propagation after short distances

    // compute distance till envelope contains ray from ballistic origin
    length_t min_ballistic_distance = calculate_min_ballistic_distance(envelope, ray);

    const auto& z_search_range = beam::beam_generic_t::major_axis_to_z_scale();

    length_t dist = 0*u::m;
    for (std::uint32_t seg=0;;++seg) {
        const auto ballistic_dist = max_ballistic_distance(lambda, seg, min_ballistic_distance);
        auto bl_intr = ads.intersect(ray, { dist, m::min(distance, dist + ballistic_dist*ballistic_scale) });
        if (!bl_intr.empty()) {
            assert(bl_intr.distance()>=dist);
            return {
                .origin = ray.o,
                .record = bl_intr,
                .ballistic = true
            };
        }

        // no intersection found. propagate beam.
        dist += ballistic_dist;
        // max ballistic attempts reached? 
        if (ballistic_dist == limits<length_t>::infinity() || dist>=distance)
            return {
                .origin = ray.o,
                .record = {},
                .ballistic = true
            };

        assert(envelope.contains(ray.propagate(dist)));

        // attempt diffusive propagation
        const auto min_df_prog = envelope.axes(dist).x / f_t(2);
        auto df_intr = ads.intersect(
                envelope,
                { dist, distance },
                { 
                    .detect_edges = opts.detect_edges,
                    .accumulate_edges = opts.accumulate_edges,
                    .z_search_range_scale = z_search_range
                });

        // successful diffusive propagation?
        if (df_intr.empty() || df_intr.distance()-dist >= min_df_prog) {
            const length_t depth = df_intr.empty() ?
                0*u::m :
                z_search_range * envelope.axes(df_intr.distance()).x;
            return {
                .origin = envelope.o(),
                .record = df_intr,
                .intersection_region_depth = depth
            };
        }

        // ... or, too short, continue ballistic path
    }

    unreachable();
}

/**
 * @brief Cone shadow query.
 */
[[nodiscard]] inline shadow_result_t traverse_shadow(
        const ads::ads_t& ads,
        const elliptic_cone_t& envelope,
        const length_t lambda,
        const length_t& distance,
        const traversal_opts_t& opts) noexcept {
    const auto& ray = envelope.ray();

    const bool ray_trace = opts.force_ray_tracing || envelope.is_ray();
    if (ray_trace) {
        // we are in pure ray tracing mode
        return { 
            .shadow = ads.shadow(ray, { 0*u::m,distance }),
            .ballistic = true
        };
    }

    // start with ballistic propagation
    // and try to resume diffusive propagation after short distances

    // compute distance till envelope contains ray from ballistic origin
    length_t min_ballistic_distance = calculate_min_ballistic_distance(envelope, ray);

    length_t dist = 0*u::m;
    for (std::uint32_t seg=0;;++seg) {
        const auto ballistic_dist = max_ballistic_distance(lambda, seg, min_ballistic_distance);
        auto bl_shadow = ads.shadow(ray, { dist, (dist+ballistic_dist)*ballistic_scale });
        if (bl_shadow) {
            return {
                .shadow = bl_shadow,
                .ballistic = true
            };
        }

        // no intersection found. propagate beam.
        dist += ballistic_dist;
        assert(envelope.contains(ray.propagate(dist)));

        // max ballistic attempts reached? 
        if (dist >= distance)
            return {
                .shadow = false,
                .ballistic = true
            };

        // attempt diffusive propagation
        const auto min_df_prog = envelope.axes(dist).x;
        auto df_shadow = ads.shadow(
                envelope,
                { dist, m::min(dist+min_df_prog, distance) });

        // no hit found?
        if (!df_shadow && dist+min_df_prog>=distance) {
            // done
            return {
                .shadow = false,
                .ballistic = false
            };
        }
        else if (!df_shadow) {
            // do a proper cone shadow query
            return {
                .shadow = ads.shadow(envelope, { dist,distance }),
                .ballistic = false
            };
        }

        // ... or, too short, continue ballistic path
    }

    unreachable();
}


using vertex_geo_variant_t = std::variant<std::monostate, pqvec3_t, intersection_surface_t>;

[[nodiscard]] inline pqvec3_t offseted_ray_origin(const vertex_geo_variant_t& geo, const ray_t& ray) noexcept {
    return std::holds_alternative<intersection_surface_t>(geo) ?
        std::get<intersection_surface_t>(geo).offseted_ray_origin(ray) :
        ray.o;
}
[[nodiscard]] inline pqvec3_t offseted_ray_origin(const Intersection auto& intrs, const ray_t& ray) noexcept {
    return intrs.offseted_ray_origin(ray);
}
[[nodiscard]] inline pqvec3_t intersection_position(const vertex_geo_variant_t& geo) noexcept {
    return std::holds_alternative<intersection_surface_t>(geo) ?
        std::get<intersection_surface_t>(geo).wp :
        std::get<pqvec3_t>(geo);
}
[[nodiscard]] inline pqvec3_t intersection_position(const Intersection auto& intrs) noexcept {
    return intrs.wp;
}


/**
 * @brief Traverses the ADS with a cone.
 *        Starts with doing short ballistic (coherent photons) segments, propagated as rays,
 *        after each segment attempt to restart diffusive (beam) propagation.
 */
[[nodiscard]] inline traversal_result_t traverse(
        const ads::ads_t& ads,
        const elliptic_cone_t &cone,
        const vertex_geo_variant_t& intrs,
        const length_t lambda,
        const length_t distance,
        const traversal_opts_t& opts = {}) noexcept {
    auto envelope = cone;
    envelope.set_o(offseted_ray_origin(intrs, cone.ray()));
    return traverse(ads, envelope, lambda, distance, opts);
}
/**
 * @brief Traverses the ADS with a cone.
 *        Starts with doing short ballistic (coherent photons) segments, propagated as rays,
 *        after each segment attempt to restart diffusive (beam) propagation.
 */
[[nodiscard]] inline traversal_result_t traverse(
        const ads::ads_t& ads,
        const elliptic_cone_t &cone,
        const vertex_geo_variant_t& intrs,
        const length_t lambda,
        const traversal_opts_t& opts = {}) noexcept {
    return traverse(ads, cone, intrs, lambda, limits<length_t>::infinity(), opts);
}

/**
 * @brief Cone shadow query.
 */
[[nodiscard]] inline shadow_result_t traverse_shadow(
        const ads::ads_t& ads,
        const elliptic_cone_t &cone,
        const vertex_geo_variant_t& intrs,
        const length_t lambda,
        const length_t distance,
        const traversal_opts_t& opts = {}) noexcept {
    auto envelope = cone;
    envelope.set_o(offseted_ray_origin(intrs, cone.ray()));
    return traverse_shadow(ads, envelope, lambda, distance, opts);
}

/**
 * @brief Ray shadow query between two intersections.
 */
[[nodiscard]] inline bool shadow(
        const ads::ads_t& ads,
        const auto& intrs_start,
        const auto& intrs_end) noexcept {
    const auto& start_wp = intersection_position(intrs_start);
    const auto& end_wp = intersection_position(intrs_end);

    const auto ray = ray_t{ start_wp, m::normalize(end_wp-start_wp) };
    const auto& o = offseted_ray_origin(intrs_start, ray);
    const auto& t = offseted_ray_origin(intrs_end, ray_t{ end_wp, -ray.d });
    const auto dist = m::length(t-o);
    const auto d = dir3_t{ (t-o)/dist };

    return ads.shadow(ray_t{ o,d }, pqrange_t<>{ 0*u::m,dist });
}

}
