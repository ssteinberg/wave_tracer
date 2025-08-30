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

#include <wt/spectrum/spectrum.hpp>
#include <wt/emitter/infinite_emitter.hpp>

#include <wt/math/common.hpp>
#include <wt/math/frame.hpp>
#include <wt/wt_context.hpp>

namespace wt::emitter {

/**
 * @brief An emitter positioned at infinity, with a fixed direction and subtending a fixed solid angle from the scene.
 */
class directional_t final : public infinite_emitter_t {
private:
    dir3_t dir_to_emitter;
    std::shared_ptr<spectrum::spectrum_real_t> spectrum;

    frame_t frame;

    // radius around world centre into which this directional emitter radiates
    length_t target_radius = {};
    // distance along dir_to_emitter from world centre to outer edge
    length_t far = {};
    // target area
    wt::area_t surface_area = {};
    // target area density
    wt::area_density_t recp_surface_area = {};

    // solid angle subtended by the source at the target centre
    f_t tan_alpha_at_traget;

protected:
    void set_world_aabb(const aabb_t& waabb) noexcept override {
        infinite_emitter_t::set_world_aabb(waabb);

        // the radiation target is the bounding 2d circle that contains the world AABB 
        // (when projected upon the plane tangent to direction to emitter).

        const auto& pr = world_aabb.extent()/2;
        const auto p00 = frame.to_local(m::mix(-pr,pr,vec3b_t{ 0,0,0 }));
        const auto p01 = frame.to_local(m::mix(-pr,pr,vec3b_t{ 0,0,1 }));
        const auto p10 = frame.to_local(m::mix(-pr,pr,vec3b_t{ 0,1,0 }));
        const auto p11 = frame.to_local(m::mix(-pr,pr,vec3b_t{ 0,1,1 }));

        const auto l00 = m::length2(pqvec2_t{ p00 });
        const auto l01 = m::length2(pqvec2_t{ p01 });
        const auto l10 = m::length2(pqvec2_t{ p10 });
        const auto l11 = m::length2(pqvec2_t{ p11 });
        const auto r2 = m::max(l00,l01,l10,l11);

        // compute target's radius and area
        target_radius = m::sqrt(r2);
        surface_area = m::pi * r2;
        recp_surface_area = f_t(1) / surface_area;
        // compute distance from world centre to outside world
        constexpr f_t scale_max = 1.01;
        far = scale_max * m::max(m::abs(p00.z), m::abs(p01.z), m::abs(p10.z), m::abs(p11.z));
    }

    [[nodiscard]] inline auto world_centre() const noexcept { return world_aabb.centre(); }

public:
    directional_t(std::string id, 
                  const dir3_t& dir, 
                  std::shared_ptr<spectrum::spectrum_real_t> irradiance,
                  solid_angle_t sa_at_traget,
                  f_t emitter_phase_space_extent_scale = 1)
        : infinite_emitter_t(std::move(id), emitter_phase_space_extent_scale), 
          dir_to_emitter(dir),
          spectrum(std::move(irradiance)),
          frame(frame_t::build_orthogonal_frame(dir_to_emitter)),
          tan_alpha_at_traget(m::tan(m::acos(1-(f_t)(sa_at_traget * m::inv_two_pi / u::ang::sr))))
    {}
    directional_t(const directional_t&) = default;
    directional_t(directional_t&&) = default;

    [[nodiscard]] inline bool is_delta_position() const noexcept override { return false; }
    [[nodiscard]] inline bool is_delta_direction() const noexcept override { return true; }

    [[nodiscard]] inline spectral_irradiance_t spectral_irradiance(const wavenumber_t k) const noexcept {
        return spectrum->f(k) * spectral_irradiance_t::unit;
    }
    
    [[nodiscard]] inline const auto& direction_to_emitter() const noexcept { return dir_to_emitter; }
    
    /**
     * @brief Returns the emitter's emission spectrum
     */
    [[nodiscard]] const spectrum::spectrum_real_t& emission_spectrum() const noexcept override {
        return *spectrum;
    }
    
    /**
     * @brief Computes total spectral power that arrives to an area.
     */
    [[nodiscard]] inline spectral_radiant_flux_t power(wavenumber_t k) const noexcept override {
        return spectral_irradiance(k) * surface_area;
    }
    /**
     * @brief Computes total power, over a wavenumber range, that arrives to an area.
     */
    [[nodiscard]] inline radiant_flux_t power(const range_t<wavenumber_t>& krange) const noexcept override {
        return spectrum->power(krange) * (f_t)(surface_area / square(u::m)) * (u::W);
    }

    [[nodiscard]] inline beam::sourcing_geometry_t sourcing_geometry(const wavenumber_t k) const noexcept {
        const auto se =
            beam::sourcing_geometry_t::source_mub_from(
                tan_alpha_at_traget,
                k
            ).phase_space_extent().enlarge(get_requested_phase_space_extent_scale());
        return
            beam::sourcing_geometry_t::source(se);
    }

    /**
     * @brief Source a beam from this light source.
     * @param k wavenumber
     */
    [[nodiscard]] spectral_irradiance_beam_t Le(
            const ray_t& p,
            const wavenumber_t k) const noexcept {
        return spectral_irradiance_beam_t{
            p, spectral_irradiance(k),
            k,
            sourcing_geometry(k)
        };
    }


    /**
     * @brief Integrate a detector beam over the emitter.
     * @param beam detection beam incident upon the emitter
     */
    [[nodiscard]] spectral_radiant_flux_stokes_t Li(
            const importance_flux_beam_t& beam,
            const intersection_surface_t* surface=nullptr) const noexcept override {
        // delta direction infinite emitter: well designed integrators should not arrive here
        assert(false);
        return {};
    }

    /**
     * @brief Samples an emission phase-space position (ray) on the light source
     * @param k wavenumber
     */
    [[nodiscard]] emitter_sample_t sample(sampler::sampler_t& sampler,
                                          const wavenumber_t k) const noexcept override;

    /**
     * @brief Samples a direct connection to a world position
     * @param wp world position
     * @param k wavenumber
     */
    [[nodiscard]] emitter_direct_sample_t sample_direct(
            sampler::sampler_t& sampler,
            const pqvec3_t& wp,
            const wavenumber_t k) const noexcept override;

    /**
     * @brief Sampling area PDF of a target scene position, sampled by the infinite emitter's `sample()`.
     * @param wp sampled scene position
     */
    [[nodiscard]] area_sampling_pd_t pdf_target_position(const pqvec3_t& wp) const noexcept override {
        assert(target_radius>zero);

        const auto plocal = pqvec2_t{ frame.to_local(wp - world_centre()) };
        return m::length2(plocal)<=m::sqr(target_radius) ? 
            recp_surface_area :
            area_density_t::zero();
    }

    /**
     * @brief Sampling PDF of an emission direction from the light source
     * @param p position on light source
     * @param dir emission direction
     * @param surface emitter surface intersection record (should be ``nullptr``)
     */
    [[nodiscard]] solid_angle_sampling_pd_t pdf_direction(
            const pqvec3_t& p,
            const dir3_t& dir, 
            const intersection_surface_t* surface=nullptr) const noexcept override {
        assert(target_radius>zero);

        return solid_angle_sampling_pd_t::discrete(dir==-dir_to_emitter ? 1 : 0);
    }

    /**
     * @brief Sampling PDF of a direct connection.
     * @param wp world position from which direct sampling was applied
     * @param sampled_r sampled emitter phase-space position (ray)
     * @param sampled_surface sampled surface for emitter samples that seat on a surface (should be ``nullptr``)
     * @return the solid angle density from ``wp``
     */
    [[nodiscard]] solid_angle_sampling_pd_t pdf_direct(
            const pqvec3_t& wp,
            const ray_t& r,
            const intersection_surface_t* surface=nullptr) const noexcept override {
        return solid_angle_sampling_pd_t::discrete(1);
    }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::unique_ptr<emitter_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);
};

}

