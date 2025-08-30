/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/wt_context.hpp>
#include <wt/scene/loader/node.hpp>
#include <wt/scene/loader/node_readers.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/bsdf/bsdf.hpp>
#include <wt/bsdf/diffuse.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace bsdf;

bsdf_result_t diffuse_t::f(const dir3_t &wi,
                           const dir3_t &wo,
                           const bsdf_query_t& query) const noexcept {
    const auto& tquery = query.intersection.texture_query(query.k);
    const auto refl = m::clamp01<f_t>(this->refl->f(tquery).x);
    return { .M = 
        (
            query.lobe[0] && wi.z>0 && wo.z>0 ?
                wo.z * m::inv_pi * refl :
                0
        ) * mueller_operator_t::perfect_depolarizer()
    };
}

std::optional<bsdf_sample_t> diffuse_t::sample(
        const dir3_t &wi,
        const bsdf_query_t& query, 
        sampler::sampler_t& sampler) const noexcept {
    if (wi.z<=0)
        return std::nullopt;

    const auto& tquery = query.intersection.texture_query(query.k);
    const auto refl = m::clamp01<f_t>(this->refl->f(tquery).x);
    const auto& wo = sampler.cosine_hemisphere();

    lobe_mask_t lobe{};
    lobe[0] = true;

    const auto pdf = solid_angle_density_t{ sampler.cosine_hemisphere_pdf(wo.z) / u::ang::sr };

    return bsdf_sample_t{
        .wo = wo,
        .dpd = pdf,

        .lobe = lobe,
 
        .weighted_bsdf = { refl * mueller_operator_t::perfect_depolarizer() },
    };
}

solid_angle_density_t diffuse_t::pdf(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept {
    return 
        query.lobe[0] && wi.z>0 && wo.z>0 ?
        solid_angle_density_t{ sampler::sampler_t::cosine_hemisphere_pdf(wo.z) / u::ang::sr } :
        solid_angle_density_t::zero();
}


scene::element::info_t diffuse_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "diffuse", {
        { "reflectance", attributes::make_element(refl.get()) },
    });
}


std::unique_ptr<bsdf_t> diffuse_t::load(std::string id, 
                                        scene::loader::loader_t* loader, 
                                        const scene::loader::node_t& node, 
                                        const wt::wt_context_t &context) {
    std::shared_ptr<texture::texture_t> reflectance;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_texture_element(item, "reflectance", reflectance, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(diffuse bsdf loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(diffuse bsdf loader) " + std::string{ exp.what() }, item);
    }
    }
    
    if (!reflectance)
        throw scene_loading_exception_t("(diffuse bsdf loader) a real 'reflectance' spectrum must be provided", node);

    return std::make_unique<diffuse_t>(std::move(id), std::move(reflectance));
}
