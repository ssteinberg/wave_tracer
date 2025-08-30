/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/scene/loader/node.hpp>
#include <wt/wt_context.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/spectrum/uniform.hpp>

#include <wt/bsdf/bsdf.hpp>
#include <wt/bsdf/surface_spm.hpp>
#include <wt/interaction/surface_profile/dirac.hpp>

#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace bsdf;


inline dir3_t flip_wo(const dir3_t& wo, const f_t eta) noexcept {
    const auto scale = wo.z>0 ? eta : 1/eta;
    const auto xy = vec2_t{ wo } * scale;
    const auto l2 = m::dot(xy,xy);

    return l2>1 ? dir3_t{ 1,0,0 } : 
                  dir3_t{ xy, f_t(wo.z>0?-1:1)*m::sqrt(m::max<f_t>(0,1-l2)) };
}

// TODO: (semi-)conductors should allow transmission and attenuation should happen in medium.
inline bool IOR_has_transmission(const c_t IOR) noexcept { return m::sqr(m::abs(IOR.imag())) / std::norm(IOR) <= f_t(1e-2); }

bsdf_result_t surface_spm_t::f(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept {
    const auto& tquery = query.intersection.texture_query(query.k);

    const bool is_scatter  = query.lobe.test(lobe_scattered) && !is_delta_only(query.k);
    const bool is_reflection = wi.z*wo.z>=0;

    const auto eta_12 = IOR(query.k);

    const bool has_transmission = IOR_has_transmission(eta_12);

    if (wi.z==0 || wo.z==0 ||
        !is_scatter ||
        (!is_reflection && !has_transmission))
        return bsdf_result_t{};

    const auto abs_wo = is_reflection ? wo : flip_wo(wo, eta_12.real());
    const auto alpha = profile->alpha(wi, abs_wo, tquery);

    f_t J=1;
    if (!is_reflection && query.transport==transport_e::backward) {
        const auto rel_eta2 = m::sqr(wi.z<0 ? 1/eta_12.real() : eta_12.real());
        J = rel_eta2;
    }

    const auto scale = is_reflection ? reflectivity_scale(query.k) : transmissivity_scale(query.k);

    // scatter by surface profile
    const auto h = vec3_t{ wi } + vec3_t{ abs_wo };
    const auto m = m::normalize(wi.z<0 ? -h : h);
    const auto F = mueller_operator_t::fresnel(eta_12.real(), is_reflection, wi, m);

    const auto psd = profile->psd(wi, abs_wo, tquery);
    return { (1-alpha) * J * m::abs(wo.z) * psd * scale * F };
}

std::optional<bsdf_sample_t> surface_spm_t::sample(
        const dir3_t &wi,
        const bsdf_query_t& query, 
        sampler::sampler_t& sampler) const noexcept {
    const auto& tquery = query.intersection.texture_query(query.k);

    const auto alpha = profile->alpha(wi, tquery);
    const bool has_specular = query.lobe.test(lobe_specular) && alpha>0;
    const bool has_scatter  = query.lobe.test(lobe_scattered) && alpha<1;

    const auto eta_12 = IOR(query.k);

    const bool has_transmission = IOR_has_transmission(eta_12);
    
    if (wi.z==0 || (!has_specular && !has_scatter)) return std::nullopt;

    // select specular or scatter interaction
    f_t pdf = 1;
    bool is_specular = has_specular;
    if (has_specular && has_scatter) {
        const auto pdf_specular = alpha;
        is_specular = pdf_specular==1 || sampler.r()<pdf_specular;
        pdf = is_specular ? pdf_specular : 1-pdf_specular;
    }

    // select reflection or transmission
    f_t J=1;

    const auto f = fresnel(eta_12, wi);
    const auto pdf_transmission = (f.Ts+f.Tp)/2;
    assert(pdf_transmission>=0 && pdf_transmission<=1);
    bool is_reflection = true;
    if (has_transmission) {
        is_reflection = sampler.r()>=pdf_transmission;
        pdf *= is_reflection ? 1-pdf_transmission : pdf_transmission;
    }

    if (!is_reflection && query.transport==transport_e::backward) {
        const auto rel_eta2 = m::sqr(std::real(f.eta_12));
        J = rel_eta2;
    }

    auto scale = is_reflection ? reflectivity_scale(query.k) : transmissivity_scale(query.k);
    if (scale==0 || (!is_reflection && !has_transmission))
        return std::nullopt;

    if (is_specular) {
        // sampled specular interaction
        lobe_mask_t lobe{};
        lobe[lobe_specular] = true;

        const auto wo = is_reflection ? reflect(wi) : f.t;
        const auto F = mueller_operator_t::fresnel(eta_12, is_reflection, wi);
        const auto M = alpha * J * scale * F;

        return bsdf_sample_t{
            .wo = wo,
            .dpd = solid_angle_sampling_pd_t::discrete(pdf),

            .eta = is_reflection ? c_t{ 1,0 } : f.eta_12,
            .lobe = lobe,
    
            .weighted_bsdf = { M/pdf },
        };
    } else {
        // sampled scatter by surface profile
        lobe_mask_t lobe{};
        lobe[lobe_scattered] = true;

        const auto sample = profile->sample(wi, tquery, sampler);
        assert(wi.z*sample.wo.z>=0);

        const auto h = vec3_t{ wi } + vec3_t{ sample.wo };
        const auto m = m::normalize(wi.z<0 ? -h : h);
        const auto F = mueller_operator_t::fresnel(eta_12, is_reflection, wi, m);

        const auto wo = is_reflection ? sample.wo : flip_wo(sample.wo, eta_12.real());

        pdf *= sample.pdf;
        const auto M = (1-alpha) * J * m::abs(wo.z) * sample.psd * scale * F;

        return bsdf_sample_t{
            .wo = wo,
            .dpd = solid_angle_density_t{ pdf / u::ang::sr },

            .eta = is_reflection ? c_t{ 1,0 } : f.eta_12,
            .lobe = lobe,
    
            .weighted_bsdf = { M/pdf },
        };
    }
}

solid_angle_density_t surface_spm_t::pdf(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept {
    const auto& tquery = query.intersection.texture_query(query.k);

    const bool is_scatter  = query.lobe.test(lobe_scattered);
    const bool is_reflection = wi.z*wo.z>=0;

    const auto eta_12 = IOR(query.k);

    const bool has_transmission = IOR_has_transmission(eta_12);

    if (wi.z==0 || wo.z==0 ||
        !is_scatter ||
        (!is_reflection && !has_transmission))
        return solid_angle_density_t::zero();

    const auto abs_wo = is_reflection ? wo : flip_wo(wo, eta_12.real());
    const auto alpha = profile->alpha(wi, tquery);

    const auto pdf_specular = query.lobe.test(lobe_specular) ? alpha : 0;
    const auto f = fresnel(eta_12.real(), wi);
    const auto pdf_transmission = (f.Ts+f.Tp)/2;

    const auto pdf = (1-pdf_specular) * profile->pdf(wi, abs_wo, tquery) * 
                     (is_reflection ? 1-pdf_transmission : pdf_transmission);
    assert(m::isfinite(pdf) && pdf>=0);

    return solid_angle_density_t{ pdf / u::ang::sr };
}


scene::element::info_t surface_spm_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "surface_spm", {
        { "IOR", attributes::make_element(IORn.get()) },
        { "ext IO", attributes::make_element(extIOR.get()) },
        { "profile", attributes::make_element(profile.get()) },
    });
}


std::unique_ptr<bsdf_t> surface_spm_t::load(std::string id, 
                                                 scene::loader::loader_t* loader, 
                                                 const scene::loader::node_t& node, 
                                                 const wt::wt_context_t &context) {
    std::shared_ptr<spectrum::spectrum_t> extIOR, IORn;
    std::shared_ptr<surface_profile::surface_profile_t> profile;
    std::shared_ptr<spectrum::spectrum_real_t> reflection_scale;
    std::shared_ptr<spectrum::spectrum_real_t> transmission_scale;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_scene_element(item, profile, loader, context) &&
            !scene::loader::load_scene_element(item, "IOR", IORn, loader, context) &&
            !scene::loader::load_scene_element(item, "extIOR", extIOR, loader, context) &&
            !scene::loader::load_scene_element(item, "reflection_scale", reflection_scale, loader, context) &&
            !scene::loader::load_scene_element(item, "transmission_scale", transmission_scale, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(surface spm bsdf loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(surface spm bsdf loader) " + std::string{ exp.what() }, item);
    }
    }
    
    if (!IORn)
        throw scene_loading_exception_t("(surface spm bsdf loader) 'IORn' spectrum must be provided", node);
    if (!profile) {
        // dirac surface profile by default
        profile = std::make_unique<surface_profile::dirac_t>(id+"_default_dirac_surface_profile");
    }

    if (!extIOR)
        extIOR = std::make_shared<spectrum::uniform_t>(id+"_default_constant_extIOR", 1);

    return std::make_unique<surface_spm_t>( 
        std::move(id), 
        std::move(extIOR), std::move(IORn), 
        std::move(profile),
        std::move(reflection_scale), std::move(transmission_scale)
    );
}
