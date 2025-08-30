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

#include <wt/util/unreachable.hpp>

#include <wt/sensor/sensor.hpp>
#include <wt/sensor/film/film.hpp>
#include <wt/sensor/mask/mask.hpp>

#include <wt/spectrum/spectrum.hpp>

#include <wt/math/common.hpp>

#include <wt/wt_context.hpp>

namespace wt {

class scene_t;

namespace sensor {

namespace detail {

// Returns a coordinate within canvas in a spiral-like order
inline auto spiral2d(std::size_t n, const vec2u32_t& canvas) {
    const auto spiral_length   = m::min_element(canvas);
    const auto spiral_elements = m::sqr(spiral_length);

    assert(spiral_length>0);

    const auto centre = vec2i32_t{ canvas.x-1,canvas.y }/2;

    if (n<spiral_elements) {
        // inside the centre spiral
        const auto r  = (std::int32_t)(m::floor((m::sqrt((f_t)n) - 1) / 2) + f_t(1.5));
        const auto p  = 4 * r*(r - 1);
        const auto en = 2*r;
        const auto a  = n==0 ? 0 : std::int32_t(n-p) % (8*r);
        const auto face = n==0 ? 0 : (int)(m::floor(a/f_t(en)) + f_t(.5));

        auto pos = vec2i32_t{};
        switch (face) {
        case 0: {
            pos[0] = a - r;
            pos[1] = -r;
            break;
        }
        case 1: {
            pos[0] = r;
            pos[1] = (a % en) - r;
            break;
        }
        case 2: {
            pos[0] = r - (a % en);
            pos[1] = r;
            break;
        }
        case 3: {
            pos[0] = -r;
            pos[1] = r - (a % en);
            break;
        }
        default:
            unreachable();
        }

        return vec2u32_t{ pos + centre };
    }
    else {
        // on the outer portion
        const auto max_dim    = (std::size_t)m::max_dimension(canvas);
        const auto spiral_min = centre[max_dim] - std::int32_t(spiral_length-1 + (max_dim==0 ? 0 : 1))/2;
        const auto spiral_max = centre[max_dim] + std::int32_t(spiral_length-1 + (max_dim==0 ? 1 : 0))/2;

        const auto idx = n-spiral_elements;
        const auto row = idx/spiral_length;
        const int x    = row%2==1 ? idx%spiral_length : spiral_length-1-idx%spiral_length;
        const int y    = row%2==1 ? spiral_max+row/2+1 : spiral_min-row/2-1;

        auto pos = vec2i32_t{ x, m::modulo(y, (int)canvas[max_dim]) };
        if (max_dim==0)
            std::swap(pos.x,pos.y);

        return vec2u32_t{ pos };
    }
}

}


template <std::size_t Dims>
class film_backed_sensor_generic_t : public sensor_t {
private:
    std::shared_ptr<mask::mask_t> sensor_mask;

public:
    film_backed_sensor_generic_t(
            std::string id,
            std::uint32_t samples_per_element,
            bool ray_trace,
            std::shared_ptr<mask::mask_t> sensor_mask = nullptr)
        : sensor_t(std::move(id), samples_per_element, ray_trace),
          sensor_mask(std::move(sensor_mask))
    {}
    film_backed_sensor_generic_t(film_backed_sensor_generic_t&&) = default;

    [[nodiscard]] inline const auto& get_sensor_mask() const noexcept { return sensor_mask; }
};


/**
 * @brief General interface for sensors supported by an underlying film of arbitrary dimensions.
 */
template <std::size_t Dims, bool polarimetric>
class film_backed_sensor_t : public film_backed_sensor_generic_t<Dims> {
    friend class scene_t;

    static_assert(Dims>=1 && Dims<=3);

public:
    using film_t = sensor::film_t<Dims, polarimetric>;
    using size_t = film_t::size_t;

private:
    film_t sensor_film;

    std::uint32_t block_size;
    size_t block_counts;

private:
    inline auto compute_block_count(const std::uint32_t block_sz, 
                                    const film_t& film) {
        return (film.dimensions() + size_t{ block_sz-1 }) / block_sz;
    }

public:
    film_backed_sensor_t(const wt_context_t &ctx, 
                         std::string id,
                         film_t film,
                         std::uint32_t samples_per_element,
                         bool ray_trace,
                         std::shared_ptr<mask::mask_t> sensor_mask = nullptr)
        : film_backed_sensor_generic_t<Dims>(std::move(id), samples_per_element, ray_trace, std::move(sensor_mask)),
          sensor_film(std::move(film)),
          block_size(ctx.renderer_block_size),
          block_counts(compute_block_count(block_size, sensor_film))
    {}
    film_backed_sensor_t(film_backed_sensor_t&&) = default;

    [[nodiscard]] inline const auto& film() const noexcept { return sensor_film; }
    [[nodiscard]] bool is_polarimetric() const noexcept final {
        return polarimetric;
    }

    [[nodiscard]] inline const response::response_t* sensor_response() const noexcept override {
        return sensor_film.response();
    }
    [[nodiscard]] inline const spectrum::spectrum_real_t& sensitivity_spectrum() const noexcept override {
        return sensor_film.response()->sensitivity();
    }

    /**
     * @brief Creates the sensor storage. Used as a render target for rendering.
     */
    [[nodiscard]] inline std::unique_ptr<film_storage_handle_t> create_sensor_film(
            const wt_context_t &context,
            sensor_write_flags_e flags) const noexcept override {
        return film().create_film_storage(context, flags);
    }
    
    /**
     * @brief Total number of sensor elements (e.g., pixels), per dimension. Returns 1 for unused dimensions.
     */
    [[nodiscard]] vec3u32_t resolution() const noexcept override {
        return vec3u32_t{ sensor_film.elements(), 1 };
    }
    
    /**
     * @brief Total number of parallel blocks available for rendering.
     */
    [[nodiscard]] std::size_t total_sensor_blocks() const noexcept override {
        return std::size_t(block_counts.x)*block_counts.y;
    }
    
    /**
     * @brief Acquires a block of sensor elements for rendering. May be not thread safe.
     * @param block_id a block index between 0 and total_sensor_blocks()
     */
    [[nodiscard]] block_handle_t acquire_sensor_block(
            const film_storage_handle_t* storage,
            std::size_t block_id) const noexcept override {
        if constexpr (Dims==3) {
            const auto blocksxy = std::size_t(block_counts.x*block_counts.y);
            const auto xy_id = block_id % blocksxy;

            const auto xy = detail::spiral2d(xy_id, vec2u32_t{ block_counts });
            const auto z = block_id / block_counts.x;

            return sensor_film.acquire_film_block(vec3u32_t{ xy,z } * block_size,
                                                  storage->get_write_flags());
        } 
        else if constexpr (Dims==2) {
            const auto xy = detail::spiral2d(block_id, block_counts);
            return sensor_film.acquire_film_block(xy * block_size,
                                                  storage->get_write_flags());
        }
        else if constexpr (Dims==1) {
            const auto x = detail::spiral2d(block_id, vec2u32_t{ block_counts.x,1 }).x;
            return sensor_film.acquire_film_block(x * block_size,
                                                  storage->get_write_flags());
        }
    }
    /**
     * @brief Releases a block post rendering. Not thread safe.
     */
    void release_sensor_block(const film_storage_handle_t* storage,
                              block_handle_t&& block) const noexcept override {
        sensor_film.release_film_block(std::move(block), storage->get_write_flags());
    }

    /**
     * @brief Splats an integrator sample onto the film storage from a thread pool worker. Used for direct sensor sampling techniques. storage must be created with sensor_flags_e::requires_direct_splats.
     * (thread safe, when accessed for a thread pool worker)
     * @param storage_ptr   film storage to splat to
     * @param element       sensor element
     * @param sample        integrator sample
     */
    void splat_direct(film_storage_handle_t* storage_ptr,
                     const sensor_element_sample_t& element,
                     const radiant_flux_stokes_t& sample,
                     const wavenumber_t k) const noexcept override {
        sensor_film.splat_direct(storage_ptr, element, sample, k);
    }
    
    /**
     * @brief Splats an integrator sample onto a film block.
     * (splatting is not thread safe)
     * @param block_handle  film block
     * @param element       sensor element
     * @param sample        integrator sample
     */
    void splat(const block_handle_t& block_handle,
               const sensor_element_sample_t& element,
               const radiant_flux_stokes_t& sample,
               const wavenumber_t k) const noexcept override {
        sensor_film.splat(block_handle, element, sample, k);
    }
};

template <std::size_t Dims>
using film_backed_sensor_scalar_t = film_backed_sensor_t<Dims, false>;
template <std::size_t Dims>
using film_backed_sensor_polarimetric_t = film_backed_sensor_t<Dims, true>;

}
}
