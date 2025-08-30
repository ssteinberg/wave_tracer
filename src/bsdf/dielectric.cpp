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
#include <wt/scene/loader/node_readers.hpp>

#include <wt/spectrum/uniform.hpp>

#include <wt/bsdf/bsdf.hpp>
#include <wt/bsdf/dielectric.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace bsdf;


std::optional<bsdf_sample_t> dielectric_t::sample(
        const dir3_t &wi,
        const bsdf_query_t& query, 
        sampler::sampler_t& sampler) const noexcept {
    const auto eta_12 = IOR(query.k);

    const auto f = fresnel(eta_12, wi);
    const auto T = (f.Ts+f.Tp)/2;
    assert(T>=0 && T<=1);

    const bool is_reflection = sampler.r()>=T;
    const auto wo  = is_reflection ? reflect(wi) : f.t;
    const auto pdf = is_reflection ? 1-T : T;

    const auto scale = is_reflection ?
        reflectivity_scale(query.k) :
        transmissivity_scale(query.k);
    if (scale==0)
        return std::nullopt;

    mueller_operator_t M;
    if (is_reflection) {
        M = scale * mueller_operator_t::fresnel(f.rs, f.rp);
    } else {
        M = f.Z * scale * mueller_operator_t::fresnel(f.ts, f.tp);

        if (query.transport==transport_e::backward) {
            const auto rel_eta2 = std::real(m::sqr(f.eta_12));
            M *= rel_eta2;
        }
    }

    assert(is_reflection || !f.TIR());

    lobe_mask_t lobe{};
    lobe[0] = true;

    return bsdf_sample_t{
        .wo = wo,
        .dpd = solid_angle_sampling_pd_t::discrete(1),

        .eta = f.eta_12,
        .lobe = lobe,
 
        .weighted_bsdf = { M/pdf },
    };
}


scene::element::info_t dielectric_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "dielectric", {
        { "IOR", attributes::make_element(IORn.get()) },
        { "ext IO", attributes::make_element(extIOR.get()) },
    });
}


std::unique_ptr<bsdf_t> dielectric_t::load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context) {
    std::shared_ptr<spectrum::spectrum_t> extIOR, IORn;
    std::shared_ptr<spectrum::spectrum_real_t> reflection_scale;
    std::shared_ptr<spectrum::spectrum_real_t> transmission_scale;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_scene_element(item, "IOR", IORn, loader, context) &&
            !scene::loader::load_scene_element(item, "extIOR", extIOR, loader, context) &&
            !scene::loader::load_scene_element(item, "reflection_scale", reflection_scale, loader, context) &&
            !scene::loader::load_scene_element(item, "transmission_scale", transmission_scale, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(dielectric bsdf loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(dielectric bsdf loader) " + std::string{ exp.what() }, item);
    }
    }
    
    if (!IORn)
        throw scene_loading_exception_t("(dielectric bsdf loader) 'IOR' spectrum must be provided", node);
    if (!extIOR)
        extIOR = std::make_shared<spectrum::uniform_t>(id+"_default_constant_extIOR", 1);

    return std::make_unique<dielectric_t>(
        std::move(id),
        std::move(extIOR), std::move(IORn),
        std::move(reflection_scale), std::move(transmission_scale)
    );
}
