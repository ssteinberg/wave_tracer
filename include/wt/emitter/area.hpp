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
#include <mutex>

#include <wt/texture/texture.hpp>
#include <wt/scene/shape.hpp>
#include <wt/emitter/emitter.hpp>

#include <wt/math/distribution/discrete_distribution.hpp>

#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

namespace wt::emitter {

/**
 * @brief An area emitter is attached to a shape, and radiates isotropically from the shape's surfaces.
 *        A texture can be used to spatially module the emitted radiance. Only textures that provide `wt::texture::texture_t::mean_spectrum()` and `wt::texture::texture_t::get_RGBA` are supported. For bitmap textures: alpha is ignored.
 *        When a texture is used, per-triangle barycentric sampling data for the given texture is constructed. This enables good sampling, but this is an expensive operation and increases loading times.
 */
class area_t final : public emitter_t {
    friend class wt::shape_t;

private:
    /** @brief Use per-triangle UV sampling data?
     *         If set to FALSE, sampling is uniform over the emitter's mesh surface and ignores the texture.
     */
    static constexpr bool use_triangle_sampling_data = true;

    using tidx_t = mesh::mesh_t::tidx_t;

    struct triangle_sampling_data_t {
        std::uint32_t texels;
        f_t recp_texels;

        area_density_t texel_to_area_density;
        discrete_distribution_t<f_t> uv_dist;
    };
    using triangle_sampler_t = std::vector<triangle_sampling_data_t>;

    struct sampling_data_t {
        discrete_distribution_t<f_t> triangle_dist;
        triangle_sampler_t triangle_samplers;

        struct triangle_sample_t {
            area_density_t ppd;
            intersection_surface_t surface;
        };

        [[nodiscard]] triangle_sample_t sample(const area_t* emitter, sampler::sampler_t& sampler) const noexcept;
        [[nodiscard]] area_density_t pdf(const area_t* emitter, const intersection_surface_t& surface) const noexcept;
    };

    std::unique_ptr<sampling_data_t> sampling_data;
    std::unique_ptr<std::mutex> samplig_data_mutex = std::make_unique<std::mutex>();

    void construct_triangle_sampling_data(const wt_context_t& ctx);

private:
    /** @brief The emitted radiance. */
    std::shared_ptr<texture::texture_t> radiance;
    /** @brief Average emitted radiance across the entire area emitter's surface. */
    std::shared_ptr<spectrum::spectrum_real_t> average_radiance;
    f_t scale = 1;

    const shape_t* shape = nullptr;

    /**
     * @brief Updates the shape associated with this area emitter.
     *        Expected to be called from friend ``shape_t`` when assigning as area emitter to a shape.
     */
    void set_shape(const wt_context_t& ctx, const shape_t* shape);

public:
    /**
     * @brief Constructs an area emitter.
     * 
     * @param radiance (optional) the emission radiance texture; if provided, must be an RGB bitmap texture.
     * @param average_radiance average emission spectrum (the radiance texture averaged across the area emitter's shape). For non-spatially varying area emitters, i.e. when no ``radiance`` texture is provided, ``average_radiance`` is used for emission.
     */
    area_t(std::string id, 
           std::shared_ptr<texture::texture_t> radiance,
           std::shared_ptr<spectrum::spectrum_real_t> average_radiance,
           f_t scale = 1,
           f_t emitter_phase_space_extent_scale = 1)
        : emitter_t(std::move(id), emitter_phase_space_extent_scale),
          radiance(std::move(radiance)),
          average_radiance(std::move(average_radiance)),
          scale(scale)
    {}
    area_t(const area_t&) = delete;
    area_t(area_t&&) = default;
    
    [[nodiscard]] inline bool is_area_emitter() const noexcept override { return true; }
    
    [[nodiscard]] inline bool is_delta_position() const noexcept override { return false; }
    [[nodiscard]] inline bool is_delta_direction() const noexcept override { return false; }

    [[nodiscard]] inline spectral_radiance_t spectral_radiance(
            const intersection_surface_t& surface,
            const wavenumber_t k) const noexcept {
        if (radiance) {
            return scale *
                   radiance->f(texture::texture_query_t{ surface.uv, k }).x *
                   spectral_radiance_t::unit;
        } else {
            return scale *
                   average_radiance->f(k) *
                   spectral_radiance_t::unit;
        }
    }
    
    /**
     * @brief Returns the emitter's average emission spectrum.
     */
    [[nodiscard]] const spectrum::spectrum_real_t& emission_spectrum() const noexcept override {
        return *average_radiance;
    }
    
    /**
     * @brief Computes total emitted spectral power.
     */
    [[nodiscard]] inline spectral_radiant_flux_t power(wavenumber_t k) const noexcept override {
        const auto area = shape->get_surface_area();
        const auto rad = average_radiance->f(k) * spectral_radiance_t::unit;
        return spectral_radiant_flux_t{ scale * rad * area * (m::pi * u::ang::sr) };
    }
    /**
     * @brief Computes total emitted power over a wavenumber range.
     */
    [[nodiscard]] inline radiant_flux_t power(const range_t<wavenumber_t>& krange) const noexcept override {
        const auto area = shape->get_surface_area();
        return scale *
               radiant_flux_t{
                   // we integrate emitted spectral radiance over a wavenumber range, and multiply by area and solid angle
                   (average_radiance->power(krange) * wavenumber_t::unit * spectral_radiance_t::unit) *
                   area * (m::pi * u::ang::sr)
               };
    }

    [[nodiscard]] inline beam::sourcing_geometry_t sourcing_geometry(const wavenumber_t k) const noexcept {
        // source from spatial extents of 10λ on the area light
        static constexpr f_t lambda_to_extent = 10;
        const auto initial_spatial_extent = 
            lambda_to_extent * wavenum_to_wavelen(k);

        const auto se =
            beam::sourcing_geometry_t::source_mub_from(
                initial_spatial_extent,
                k
            ).phase_space_extent().enlarge(get_requested_phase_space_extent_scale());
        return
            beam::sourcing_geometry_t::source(se);
    }

    /**
     * @brief Source a beam from the emitter.
     * @param r desired emission phase-space position (ray) on the light source
     * @param k wavenumber
     * @param surface surface at emission point
     */
    [[nodiscard]] inline spectral_radiance_beam_t Le(
            const ray_t& r, 
            const wavenumber_t k,
            const intersection_surface_t* surface) const noexcept {
        // TODO: sourcing should take emitter shape into account
        return spectral_radiance_beam_t{
            r, spectral_radiance(*surface, k) * m::max<f_t>(0, m::dot(r.d, surface->ng())),
            k, sourcing_geometry(k)
        };
    }

    /**
     * @brief Integrate a detector beam over the emitter.
     * @param beam detection beam incident upon the emitter
     */
    [[nodiscard]] spectral_radiant_flux_stokes_t Li(
            const importance_flux_beam_t& beam,
            const intersection_surface_t* surface=nullptr) const noexcept override;

    /**
     * @brief Samples a emission phase-space position (ray) on the light source
     * @param k wavenumber
     */
    [[nodiscard]] emitter_sample_t sample(sampler::sampler_t& sampler,
                                          const wavenumber_t k) const noexcept override;

    /**
     * @brief Samples an emission phase-space position (ray) on the light source
     */
    [[nodiscard]] position_sample_t sample_position(sampler::sampler_t& sampler) const noexcept override;

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
     * @brief Sampling PDF of an emission position on the light source
     * @param p position on light source
     * @param surface emitter surface intersection record
     */
    [[nodiscard]] area_sampling_pd_t pdf_position(
            const pqvec3_t& p,
            const intersection_surface_t* surface=nullptr) const noexcept override;
    /**
     * @brief Sampling PDF of an emission direction from the light source
     * @param p position on light source
     * @param dir emission direction
     * @param surface emitter surface intersection record
     */
    [[nodiscard]] solid_angle_sampling_pd_t pdf_direction(
            const pqvec3_t& p,
            const dir3_t& dir, 
            const intersection_surface_t* surface=nullptr) const noexcept override;

    /**
     * @brief Sampling PDF of a direct connection.
     * @param wp world position from which direct sampling was applied
     * @param sampled_r sampled emitter phase-space position (ray)
     * @param sampled_surface sampled surface for emitter samples that seat on a surface (required)
     * @return the solid angle density from ``wp``
     */
    [[nodiscard]] solid_angle_sampling_pd_t pdf_direct(
            const pqvec3_t& wp,
            const ray_t& r,
            const intersection_surface_t* surface=nullptr) const noexcept override;

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::unique_ptr<emitter_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);
};

}

