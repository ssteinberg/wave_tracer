/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <string>

#include <wt/texture/texture.hpp>
#include <wt/texture/quantity.hpp>

#include <wt/math/common.hpp>
#include <wt/interaction/surface_profile/surface_profile.hpp>
#include <wt/interaction/surface_profile/fractal.hpp>

namespace wt::surface_profile {

namespace detail {

// Truncated Box-Mueller transform
inline std::pair<vec2_t,f_t> sample_boxmueller_truncated(vec2_t sample, vec2_t mean, f_t sigma2) noexcept {
    const auto eps = limits<f_t>::epsilon();

    const auto l = m::sqrt(m::min<f_t>(1,dot(mean,mean)));
    const auto coso = m::sqrt(m::max<f_t>(0,1-m::dot(mean,mean)));
    const auto phi_i = mean.x!=0 || mean.y!=0 ? m::atan2(mean.y,mean.x) : 0*u::ang::rad;

    const auto s = m::exp(-f_t(.5)*m::sqr(1+l)/sigma2);
    const auto x = (1-s)*m::max<f_t>(eps,sample.x)+s;
    const auto r = m::sqrt(-2 * sigma2 * m::log(x));

    const auto max_phi = 
        r<eps||l<eps ? 
        m::pi * u::ang::rad :
        m::max(
            f_t(1e-2) * u::ang::rad,
            m::acos(m::clamp<f_t>((m::sqr(r)+m::sqr(l)-1)/(2*r*l),-1,1))
        );

    const auto phi = phi_i + m::pi*u::ang::rad + max_phi*(2*sample.y-1);
    const vec2_t p  = r*vec2_t{ m::cos(phi),m::sin(phi) };

    const auto pdf = f_t(.5) * x/(max_phi*sigma2) * coso;

    return std::make_pair(p + mean, (f_t)(pdf * u::ang::rad));
}
inline f_t boxmueller_truncated_pdf(vec2_t wo, vec2_t mean, f_t sigma2) noexcept {
    const auto eps = limits<f_t>::epsilon();

    const auto l = m::sqrt(m::min<f_t>(1,m::dot(mean,mean)));
    const auto coso = m::sqrt(m::max<f_t>(0,1-m::dot(mean,mean)));

    wo -= mean;
    const auto r2 = m::dot(wo,wo);
    const auto x  = m::exp(f_t(-.5) * r2/sigma2);

    const auto r  = m::sqrt(r2);
    const auto max_phi = 
        r<eps||l<eps ? 
        m::pi * u::ang::rad :
        m::max(
            f_t(1e-2) * u::ang::rad,
            m::acos(m::clamp<f_t>((m::sqr(r)+m::sqr(l)-1)/(2*r*l),-1,1))
        );

    return (f_t)(f_t(.5) * x/(max_phi*sigma2) * coso * u::ang::rad);
}

}

/**
 * @brief Surface with gaussian statistics. 
 *        Implements sampling and evaluation.
 */
class gaussian_t final : public surface_profile_t {
private:
    bool roughness_parametrized;
    std::shared_ptr<texture::texture_t> roughness_tex;
    std::unique_ptr<texture::quantity_t<rms_t>> sigma_tex;

    [[nodiscard]] inline auto sigma2_normalized(const auto& sigma2, const wavenumber_t k) const noexcept {
        return 1/(1 - m::exp(-f_t(k*k/2/sigma2)));
    }

    struct params_t {
        rms2_t sigma2;
        f_t sigma2_norm;
        f_t alpha;
    };
    [[nodiscard]] inline auto gaussian_params(const texture_query_t& query) const noexcept {
        constexpr auto meank = wavelen_to_wavenum(550 * u::nm);

        if (roughness_parametrized) {
            const auto roughness = roughness_tex->f(query).x;
            const auto T = fractal_profile_details::roughness_to_T(roughness, meank);
            const auto sigma2 = 1/T;
            const auto sigma2_norm = this->sigma2_normalized(sigma2, query.k);
            return params_t{
                .sigma2 = sigma2,
                .sigma2_norm = sigma2_norm,
                .alpha = fractal_profile_details::roughness_to_alpha(roughness),
            };
        } else {
            const auto sigma2 = m::sqr(sigma_tex->f(query));
            const auto sigma2_norm = this->sigma2_normalized(sigma2, query.k);
            return params_t{
                .sigma2 = sigma2,
                .sigma2_norm = sigma2_norm,
                .alpha = (f_t)(sigma2 * square(u::mm))
            };
        }
    }

    template <QuantityOf<inverse(isq::length)> Qz>
    [[nodiscard]] inline f_t psd(const params_t& params,
                                 const qvec2<Qz>& z, 
                                 const Wavenumber auto k) const noexcept {
        const auto z2 = m::dot(z,z);
        const auto e  = m::exp(-(f_t)(z2/2 / params.sigma2));
        return e<=limits<f_t>::epsilon() ? 0 :
            params.sigma2_norm * (f_t)(m::inv_two_pi/params.sigma2 * k*k * e);
    }

public:
    gaussian_t(std::string id, 
               std::unique_ptr<texture::quantity_t<rms_t>>&& sigma) noexcept 
        : surface_profile_t(std::move(id)),
          roughness_parametrized(false),
          sigma_tex(std::move(sigma))
    {}
    gaussian_t(std::string id, 
               std::shared_ptr<texture::texture_t>&& roughness) noexcept 
        : surface_profile_t(std::move(id)),
          roughness_parametrized(true),
          roughness_tex(std::move(roughness))
    {}

    /**
     * @brief Variance of the surface profile.
     */
    [[nodiscard]] variance_t variance(const texture_query_t& query) const noexcept override {
        const auto& params = gaussian_params(query);
        return params.sigma2;
    }

    /**
     * @brief RMS roughness of the surface profile.
     */
    [[nodiscard]] rms_t rms_roughness(const texture_query_t& query) const noexcept override {
        const auto& params = gaussian_params(query);
        return m::sqrt(params.sigma2);
    }

    /**
     * @brief Fraction of scatter contained in specular term.
     * @param k wavenumber
     */
    [[nodiscard]] f_t alpha(const dir3_t& wi,
                            const dir3_t& wo,
                            const texture_query_t& query) const noexcept override {
        const auto& params = gaussian_params(query);
        const auto a = m::sqr((m::abs(wi.z)+m::abs(wo.z)) * (f_t)(query.k * u::mm)) * params.alpha;
        return m::exp(-a);
    }
    /**
     * @brief Fraction of scatter contained in specular term.
     * @param k wavenumber
     */
    [[nodiscard]] f_t alpha(const dir3_t& wi,
                            const texture_query_t& query) const noexcept override {
        return alpha(wi, wi, query);
    }

    [[nodiscard]] bool is_delta_only(wavenumber_t k) const noexcept override {
        return roughness_parametrized ? 
            roughness_tex->mean_value(k)==0 :
            sigma_tex->mean_value(k)==zero;
    }
    
    /**
     * @brief Returns true for profiles that make use of the surface interaction footprint data
     */
    [[nodiscard]] bool needs_interaction_footprint() const noexcept override {
        return roughness_parametrized ?
            roughness_tex->needs_interaction_footprint() :
            sigma_tex->needs_interaction_footprint();
    }

    /**
     * @brief Evaluates the gaussian lobe power spectral density (PSD).
     */
    [[nodiscard]] f_t psd(const dir3_t& wi,
                          const dir3_t& wo,
                          const texture_query_t& query) const noexcept override {
        const auto& k = query.k;
        const auto& params = gaussian_params(query);
        const auto& z = k * (vec2_t{ wi } + vec2_t{ wo });
        return psd(params, z, k);
    }

    /**
     * @brief Samples a scattered direction.
     */
    [[nodiscard]] surface_profile_sample_ret_t sample(
            const dir3_t& wi,
            const texture_query_t& query,
            sampler::sampler_t& sampler) const noexcept override {
        const auto& k = query.k;
        const auto& params = gaussian_params(query);

        const auto s2 = (f_t)(params.sigma2 / (k*k));
        const auto mean = -vec2_t{ wi };
        const auto sample = detail::sample_boxmueller_truncated(sampler.r2(), mean, s2);

        const auto wo2 = sample.first;
        const auto pdf = sample.second;
        const auto psd = this->psd(params, k*(wo2-mean), k);

        assert(m::isfinite(psd) && m::isfinite(pdf));

        const auto z = m::sqrt(m::max<f_t>(0,1-m::dot(wo2,wo2)));
        return surface_profile_sample_ret_t{
            .wo = dir3_t{ wo2, wi.z>=0?z:-z },
            .pdf = pdf,
            .psd = psd,
            .weight = psd/pdf,
        };
    }

    /**
     * @brief Provides the sampling density. 
     */
    [[nodiscard]] f_t pdf(const dir3_t& wi,
                          const dir3_t& wo,
                          const texture_query_t& query) const noexcept override {
        const auto& k = query.k;
        const auto& params = gaussian_params(query);

        const auto s2 = (f_t)(params.sigma2 / (k*k));
        const auto mean = -vec2_t{ wi };
        return detail::boxmueller_truncated_pdf(vec2_t{ wo }, mean, s2);
    }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::unique_ptr<surface_profile_t> load(std::string id, scene::loader::loader_t* loader, const scene::loader::node_t& node, const wt::wt_context_t &context);
};

}
