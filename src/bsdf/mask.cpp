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
#include <wt/bsdf/mask.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace bsdf;

bsdf::bsdf_result_t mask_t::f(const dir3_t &wi,
                              const dir3_t &wo,
                              const bsdf_query_t& query) const noexcept {
    const auto& tquery = query.intersection.texture_query(query.k);
    const auto alpha = m::clamp01<f_t>(this->mask->f(tquery).x);
    if (alpha==0)
        return {};
    auto nestedf = nested->f(wi, wo, query);
    nestedf.M *= alpha;
    return nestedf;
}

std::optional<bsdf_sample_t> mask_t::sample(
        const dir3_t &wi,
        const bsdf_query_t& query, 
        sampler::sampler_t& sampler) const noexcept {
    const auto null_lobe = this->lobe_null(query.k);
    const bool has_null = query.lobe.test(null_lobe);

    // no transmission on masked nested bsdf
    if (wi.z<=0 && !has_null) return std::nullopt;

    const auto& tquery = query.intersection.texture_query(query.k);
    const auto alpha = has_null ? m::clamp01<f_t>(this->mask->f(tquery).x) : f_t(1);
    const auto pdf_null = wi.z<=0 ? 1 : has_null ? 1-alpha : 0;
    const auto is_null = 
        pdf_null==0 ? false :
        pdf_null==1 ? true :
        sampler.r()<pdf_null;

    if (is_null) {
        return bsdf_sample_t{
            .wo = -wi,
            .dpd = solid_angle_sampling_pd_t::discrete(pdf_null),

            .lobe = null_lobe,
    
            .weighted_bsdf = { (1-alpha) / pdf_null * mueller_operator_t::identity() },
        };
    }

    auto nested_sample = nested->sample(wi, query, sampler);
    nested_sample->dpd *= 1-pdf_null;
    nested_sample->weighted_bsdf.M *= alpha / (1-pdf_null);
    return nested_sample;
}

solid_angle_density_t mask_t::pdf(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept {
    // no transmission on masked nested bsdf
    if (wi.z<=0 || wo.z<=0)
        return {};

    const auto null_lobe = this->lobe_null(query.k);
    const bool has_null = query.lobe.test(null_lobe);

    const auto& tquery = query.intersection.texture_query(query.k);
    const auto alpha = has_null ? m::clamp01<f_t>(this->mask->f(tquery).x) : f_t(1);
    const auto pdf_null = has_null ? 1-alpha : 0;

    return 
        nested->pdf(wi,wo,query) * (1-pdf_null);
}


scene::element::info_t mask_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "mask", {
        { "mask", attributes::make_element(mask.get()) },
        { "nested", attributes::make_element(nested.get()) },
    });
}


std::unique_ptr<bsdf_t> mask_t::load(std::string id, 
                                        scene::loader::loader_t* loader, 
                                        const scene::loader::node_t& node, 
                                        const wt::wt_context_t &context) {
    std::shared_ptr<texture::texture_t> mask;
    std::shared_ptr<bsdf_t> bsdf;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_texture_element(item, "mask", mask, loader, context) &&
            !scene::loader::load_scene_element(item, bsdf, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(mask bsdf loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(mask bsdf loader) " + std::string{ exp.what() }, item);
    }
    }
    
    if (!mask)
        throw scene_loading_exception_t("(mask bsdf loader) a real 'mask' spectrum must be provided", node);
    if (!bsdf)
        throw scene_loading_exception_t("(mask bsdf loader) 'mask' bsdf must contain a nested bsdf", node);

    return std::make_unique<mask_t>(std::move(id), std::move(mask), std::move(bsdf));
}
