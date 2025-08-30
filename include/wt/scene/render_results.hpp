/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <chrono>
#include <memory>
#include <variant>

#include <wt/sensor/film/defs.hpp>
#include <wt/sensor/sensor.hpp>
#include <wt/bitmap/bitmap.hpp>
#include "wt/bitmap/common.hpp"

namespace wt::scene {

template <std::size_t Dims>
struct developed_scalar_film_pair_t {
    /** @brief Tonemapped developed film.
     */
    std::unique_ptr<sensor::developed_scalar_film_t<Dims>> developed_tonemapped;
    /** @brief Colour encoding of tonemapped developed film.
     */
    bitmap::colour_encoding_t tonemapped_film_colour_encoding;

    /** @brief Developed film without tonemapping operator applied.
     *         Colour encoding of this film is always linear.
     */
    std::unique_ptr<sensor::developed_scalar_film_t<Dims>> developed;
};
template <std::size_t Dims>
struct developed_polarimetric_film_pair_t {
    /** @brief Tonemapped developed film.
     */
    std::unique_ptr<sensor::developed_polarimetric_film_t<Dims>> developed_tonemapped;
    /** @brief Colour encoding of tonemapped developed film.
     */
    bitmap::colour_encoding_t tonemapped_film_colour_encoding;

    /** @brief Developed film without tonemapping operator applied.
     *         Colour encoding of this film is always linear.
     */
    std::unique_ptr<sensor::developed_polarimetric_film_t<Dims>> developed;
};

/**
 * @brief Helper structure that holds a scene's rendering results.
 */
struct sensor_render_result_t {
    const sensor::sensor_t* sensor;
    std::chrono::high_resolution_clock::duration render_elapsed_time{};

    using developed_films_t = std::variant<
        /** @brief One-dimensional scalar films. */
        developed_scalar_film_pair_t<1>,
        /** @brief Two-dimensional scalar films. */
        developed_scalar_film_pair_t<2>,
        /** @brief Three-dimensional scalar films. */
        developed_scalar_film_pair_t<3>,
        /** @brief One-dimensional polarimetric (Stokes) films. */
        developed_polarimetric_film_pair_t<1>,
        /** @brief Two-dimensional polarimetric (Stokes) films. */
        developed_polarimetric_film_pair_t<2>,
        /** @brief Three-dimensional polarimetric (Stokes) films. */
        developed_polarimetric_film_pair_t<3>
    >;
    /** @brief Variant holding the developed film; held type depends on sensor response. */
    developed_films_t developed_films;

    std::size_t spe_written;
    /** @brief For partial results, this may be a non integer (average over all written blocks). */
    std::optional<f_t> fractional_spe;
};
struct render_result_t {
    std::unordered_map<std::string, sensor_render_result_t> sensors;
    std::chrono::high_resolution_clock::duration render_elapsed_time{};
};

}
