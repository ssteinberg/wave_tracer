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

#include <wt/bsdf/two_sided.hpp>

#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace bsdf;


constexpr inline auto flip(const dir3_t& w, const f_t z) noexcept {
    return z>=0 ? w : dir3_t{ w.x,w.y,-w.z };
}

bsdf_result_t two_sided_t::f(const dir3_t &wi,
                             const dir3_t &wo,
                             const bsdf_query_t& query) const noexcept {
    return nested->f(flip(wi,wi.z),
                     flip(wo,wi.z),
                     query);
}

std::optional<bsdf_sample_t> two_sided_t::sample( 
        const dir3_t &wi,
        const bsdf_query_t& query, 
        sampler::sampler_t& sampler) const noexcept {
    auto sample = nested->sample(flip(wi,wi.z),
                                 query, sampler);
    if (sample)
        sample->wo = flip(sample->wo,wi.z);
    return sample;
}

solid_angle_density_t two_sided_t::pdf(
        const dir3_t &wi,
        const dir3_t &wo,
        const bsdf_query_t& query) const noexcept {
    return nested->pdf(flip(wi,wi.z),
                       flip(wo,wi.z),
                       query);
}

f_t two_sided_t::eta(const dir3_t &wi,
                     const dir3_t &wo,
                     const wavenumber_t k) const noexcept {
    return nested->eta(flip(wi,wi.z),flip(wo,wi.z),k);
}


scene::element::info_t two_sided_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "two_sided", {
        { "nested", attributes::make_element(nested.get()) },
    });
}


std::unique_ptr<bsdf_t> two_sided_t::load(std::string id, 
                                    scene::loader::loader_t* loader, 
                                    const scene::loader::node_t& node, 
                                    const wt::wt_context_t &context) {
    std::shared_ptr<bsdf_t> bsdf;

    for (auto& item : node.children_view()) {
        if (!scene::loader::load_scene_element(item, bsdf, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(twosided bsdf loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    }

    if (!bsdf)
        throw scene_loading_exception_t("(twosided bsdf loader) 'twosided' bsdf must contain a nested bsdf", node);

    return std::make_unique<two_sided_t>(std::move(id), std::move(bsdf));
}
