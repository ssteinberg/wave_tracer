/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/interaction/fsd/free_space_diffraction.hpp>
#include <wt/interaction/fsd/utd.hpp>

#include <wt/ads/intersection_record.hpp>
#include <wt/math/intersect/misc.hpp>
#include <wt/math/common.hpp>

using namespace wt;


static constexpr f_t utd_IS_sigma_scale = 45;


free_space_diffraction_t::free_space_diffraction_t(
        const ads::ads_t* ads,
        const pqvec3_t& interaction_wp,
        const frame_t&  interaction_region_frame,
        const pqvec3_t& interaction_region_size,
        const dir3_t& wi,
        wavenumber_t k,
        const ads::intersection_record_t::edges_container_t& edges) noexcept
    : interaction_wp(interaction_wp) 
{
    // build aperture
    auto aperture = utd::fsd_aperture_t{ .k = k };
    aperture.edges.reserve(edges.size());
    for (const auto& ed : edges) {
        const auto& edge = ads->edge(ed);

        const auto f1_is_front = m::dot(wi,edge.n1)>zero;
        const auto& nff = f1_is_front ? edge.n1 : edge.n2;
        const auto& tff = f1_is_front ? edge.t1 : edge.t2;
        const auto& nbf = f1_is_front ? edge.n2 : edge.n1;

        // light incident from inside the wedge?
        if (m::dot(wi, nff)<=zero)
            continue;

        const auto alpha = edge.alpha;

        // clamp edges to interaction region
        auto v1 = edge.a, v2 = edge.b;
        if (m::isfinite(interaction_region_size)) {
            const auto intr = intersect::intersect_edge_ellipsoid(
                    edge.a,edge.b,
                    interaction_wp, interaction_region_frame.t, interaction_region_frame.b,
                    interaction_region_size);
            const auto t1 = m::clamp01(intr.t1);
            const auto t2 = m::clamp01(intr.t2);            
            v1 = m::mix(edge.a,edge.b,t1);
            v2 = m::mix(edge.a,edge.b,t2);
        }
        assert(m::isfinite(v1) && m::isfinite(v2));
        if (v1==v2)
            continue;

        const auto& v  = (v1+v2)/2;
        const auto& l  = m::length(v2-v1);

        aperture.edges.emplace_back(utd::wedge_edge_t{
            .v = v,
            .l = l,
            .nff = nff,
            .tff = tff,
            .nbf = nbf,
            .alpha = alpha,
            .eta = 0,
            .ads_edge_idx = ed,
        });
    }

    this->aperture = aperture;
}

free_space_diffraction_t::sample_ret_t free_space_diffraction_t::sample(
        const ads::ads_t* ads,
        const pqvec3_t& src,
        sampler::sampler_t& sampler) const noexcept {
    const auto eidx = sampler.uniform_int_interval(0,aperture.edges.size()+1);

    if (eidx == aperture.edges.size()) {
        // sampled direct term
        const auto wi = m::normalize(src-interaction_wp);
        return {
            .diffraction_wp = interaction_wp,
            .wo  = -wi,
            .is_direct = true,
            .dpd = angle_sampling_pd_t::discrete(1 / f_t(aperture.edges.size()+1)),
            .weight = f_t(aperture.edges.size()+1),
        };
    }

    // sampled edge
    const auto& edge = aperture.edges[eidx];

    const auto p = edge.v + (sampler.r()-f_t(.5)) * edge.l * edge.e();
    const auto ui = src-p;

    if (m::dot(ui,edge.nff)<=zero && m::dot(ui,edge.nbf)<=zero)
        return {};

    const auto ri = m::length(src-p);
    const auto wi = dir3_t{ (src-p)/ri };

    const auto phii = m::atan2(m::dot(edge.nff,wi), m::dot(edge.tff,wi));

    const auto sigma = m::sqrt(utd_IS_sigma_scale / u::to_num(aperture.k * ri)) * u::ang::rad;
    const auto sample = sigma * sampler.normal2d().x;

    const auto mean_phi1 = m::pi*u::ang::rad + phii;
    const auto mean_phi2 = m::pi*u::ang::rad - phii;
    const auto phio = (sampler.r()<f_t(.5) ? mean_phi1 : mean_phi2) + sample;

    auto x1 = m::abs(m::mod(phio - mean_phi1, m::two_pi));
    auto x2 = m::abs(m::mod(phio - mean_phi2, m::two_pi));
    if (x1>m::pi*u::ang::rad) x1-=m::two_pi*u::ang::rad;
    if (x2>m::pi*u::ang::rad) x2-=m::two_pi*u::ang::rad;

    const auto e = edge.e();
    const auto cos_beta = m::dot(wi,e);
    const auto sin_beta = m::sqrt(m::max<f_t>(0,1-m::sqr(cos_beta)));
    const auto wo = dir3_t{ sin_beta * (m::cos(phio)*edge.tff + m::sin(phio)*edge.nff) - cos_beta * e };

    if (m::dot(wo,edge.nff)<=0 && m::dot(wo,edge.nbf)<=0)
        return {};
    // avoid grazing angles
    if (sin_beta < utd::utd_min_sin_beta)
        return {};

    const auto intersection = intersection_edge_t(ads->edge(edge.ads_edge_idx), p);
    const auto dpd = this->pdf(src, wo);
    if (dpd == zero)
        return {};

    return {
        .diffraction_wp = p,
        .wo = wo,
        .intersection = intersection,
        .is_direct = false,
        .dpd = dpd,
        .weight = u::to_rad(f_t(1) / dpd.density()),
    };
}

angle_sampling_pd_t free_space_diffraction_t::pdf(const pqvec3_t& src, const dir3_t& wo) const noexcept {
    if (aperture.edges.size()==0)
        return {};

    angle_density_t ret{};
    for (const auto& edge : aperture.edges) {
        const auto p = edge.diffraction_point(src, wo);
        if (!p)
            continue;

        const auto ui = src-*p;
        if ((m::dot(wo,edge.nff)<=zero && m::dot(wo,edge.nbf)<=zero) ||
            (m::dot(ui,edge.nff)<=zero && m::dot(ui,edge.nbf)<=zero))
            continue;

        const auto ri = m::length(src-*p);
        const auto wi = dir3_t{ (src-*p)/ri };

        const auto phii = m::atan2(m::dot(edge.nff,wi), m::dot(edge.tff,wi));
        const auto phio = m::atan2(m::dot(edge.nff,wo), m::dot(edge.tff,wo));
        const auto sigma = m::sqrt(utd_IS_sigma_scale / u::to_num(aperture.k * ri)) * u::ang::rad;

        const auto mean_phi1 = m::pi*u::ang::rad + phii;
        const auto mean_phi2 = m::pi*u::ang::rad - phii;
        auto x1 = m::abs(m::mod(phio - mean_phi1, m::two_pi));
        auto x2 = m::abs(m::mod(phio - mean_phi2, m::two_pi));
        if (x1>m::pi*u::ang::rad) x1-=m::two_pi*u::ang::rad;
        if (x2>m::pi*u::ang::rad) x2-=m::two_pi*u::ang::rad;

        // angle density along the Keller cone
        const auto apd = m::inv_sqrt_two_pi/sigma * (
            m::exp(-f_t(.5) * m::sqr(x1/sigma)) +
            m::exp(-f_t(.5) * m::sqr(x2/sigma))
        )/2;

        ret += apd;
    }

    return angle_sampling_pd_t{ ret / (f_t)(aperture.edges.size()+1) };
}

free_space_diffraction_t::eval_ret_t free_space_diffraction_t::f(
        const pqvec3_t& src,
        const pqvec3_t& dst) const noexcept {
    eval_ret_t ret;

    const auto& k = aperture.k;

    ret.reserve(aperture.edges.size());
    for (const auto& e : aperture.edges) {
        const auto p = e.diffraction_point(src, dst);
        if (!p)
            continue;

        const auto ui = src-*p;
        const auto uo = dst-*p;
        // ignore into wedge rays
        if ((m::dot(uo,e.nff)<=zero && m::dot(uo,e.nbf)<=zero) ||
            (m::dot(ui,e.nff)<=zero && m::dot(ui,e.nbf)<=zero))
            continue;

        const auto ri = m::length(ui);
        const auto ro = m::length(uo);
        const auto wi = dir3_t{ ui/ri };
        const auto wo = dir3_t{ uo/ro };
        const auto f = e.UTD(k, wi, wo, ro);
        if (f.Dh==zero && f.Ds==zero)
            continue;

        ret.emplace_back(diffracting_edge_t{
            .utd = f,
            .edge_idx = e.ads_edge_idx,
            .p = *p,
            .wi = wi,
            .wo = wo,
            .ri = ri,
            .ro = ro,
        });
    }

    return ret;
}