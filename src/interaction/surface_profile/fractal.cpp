/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <optional>

#include <wt/interaction/surface_profile/fractal.hpp>
#include <wt/texture/constant.hpp>

#include <wt/math/common.hpp>

#include <wt/scene/loader/node.hpp>
#include <wt/scene/element/attributes.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::surface_profile;


surface_profile_sample_ret_t fractal_t::sample(
        const dir3_t& wi,
        const texture_query_t& query,
        sampler::sampler_t& sampler) const noexcept {
    // Using the importance sampling strategy from
    // "A Two-Scale Microfacet Reflectance Model Combining Reflection and Diffraction", HOLZSCHUCH & PACANOWSKI 2017

    const auto& k = query.k;
    const auto params = fractal_params(query);

    const auto s = m::sqrt(m::max<f_t>(0,1-m::sqr(wi.z)));
    const auto phi_i = s>0 ? m::atan2(wi.y,wi.x) : 0 * u::ang::rad;
    const auto sqrtT = m::sqrt(params.T);

    const auto u2 = sampler.r2();

    const auto k2T = u::to_num(m::sqr(k) * params.T);
    const auto M = 1 - m::pow(1 + k2T*m::sqr(1+s), -(gamma-1)/2);
    const auto f = m::sqrt(m::pow(1-M*u2.x, -2/(gamma-1))-1) / sqrtT;
    const auto f_k = u::to_num(f/k);

    const auto phi_max = f==zero || s==zero ?
        m::pi * u::ang::rad :
        m::acos(m::clamp<f_t>((m::sqr(f_k) + m::sqr(s) - 1)/(2*f_k*s), -1,1));
    const auto phi_f = phi_i + (2*u2.y-1)*phi_max;
    const auto vf = f * vec2_t{ m::cos(phi_f),m::sin(phi_f) };

    const auto zeta = vf;
    const auto zeta_k = u::to_num(zeta/k);
    const auto wo = zeta_k - vec2_t{ wi.x,wi.y };
    const f_t z = m::sqrt(m::max<f_t>(0,1-m::dot(wo,wo)));

    const auto psd = this->psd(params, zeta, k);
    const auto w   = m::inv_pi * (phi_max/u::ang::rad);
    const auto pdf = w>f_t(1e-2) ? z * psd / w : 0;

    return surface_profile_sample_ret_t{
        .wo  = dir3_t{ wo, wi.z>=0 ? z : -z },
        .pdf = (f_t)pdf,
        .psd = psd,
        .weight = (f_t)w,
    };
}


scene::element::info_t fractal_t::description() const {
    using namespace scene::element;

    auto ret = info_for_scene_element(*this, "fractal", {
        { "log-log slope (γ)", attributes::make_scalar(gamma) }
    });

    if (roughness_parametrized) {
        ret.attribs.emplace("roughness", attributes::make_element(roughness_tex.get()));
    } else {
        ret.attribs.emplace("correlation length (T)", attributes::make_element(T_tex.get()));
        ret.attribs.emplace("RMS roughness", attributes::make_element(sigmah_tex.get()));
    }

    return ret;
}

std::unique_ptr<surface_profile_t> fractal_t::load(std::string id, 
                                                   scene::loader::loader_t* loader, 
                                                   const scene::loader::node_t& node, 
                                                   const wt::wt_context_t &context) {
    std::optional<f_t> gamma;
    std::shared_ptr<texture::texture_t> roughness_tex;
    std::unique_ptr<texture::quantity_t<fractalT_t>> T_tex;
    std::unique_ptr<texture::quantity_t<rms_t>> sigmah_tex;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_texture_element(item, "roughness", roughness_tex, loader, context) &&
            !scene::loader::load_quantity_texture_element<fractalT_t>(id+"_T", item, "T", T_tex, loader, context) &&
            !scene::loader::load_quantity_texture_element<rms_t>(id+"_sigmah", item, "sigma_h", sigmah_tex, loader, context) &&
            !scene::loader::read_attribute(item, "gamma", gamma))
            logger::cwarn()
                << loader->node_description(item)
                << "(fractal surface_profile loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(fractal surface_profile loader) " + std::string{ exp.what() }, item);
    }
    }

    const bool has_T = !!T_tex && !!sigmah_tex;
    const bool has_roughness = !!roughness_tex;
    if ((!has_T && !has_roughness) || ((!!T_tex || !!sigmah_tex) && has_roughness))
        throw scene_loading_exception_t("(fractal surface_profile loader) Either 'roughness' or the pair 'T', 'sigma_h' must be provided", node);

    if (!gamma)
        gamma = f_t(3);
    gamma = m::max<f_t>(1.1,*gamma);

    return !!roughness_tex ?
        std::make_unique<fractal_t>(std::move(id), *gamma, std::move(roughness_tex)) :
        std::make_unique<fractal_t>(std::move(id), *gamma, std::move(T_tex), std::move(sigmah_tex));
}
