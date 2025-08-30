/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <type_traits>
#include <wt/bitmap/bitmap.hpp>

namespace wt::sensor {

/**
 * @brief Controls the internal film storage type.
 *        When TRUE, use double-precision values for accurate accumulation of very many samples.
 *        film_storage_t::develop() downcasts to the desiredPixelT=f_t type (single precision).
 */
static constexpr bool use_double_precision_for_internal_storage = true;

using PixelT = float;
using FilmStoragePixelT = std::conditional_t<use_double_precision_for_internal_storage,
                                             double, PixelT>;

template <std::size_t Dims>
using developed_scalar_film_t = bitmap::bitmap_t<PixelT,Dims>;
template <std::size_t Dims>
using developed_polarimetric_film_t = std::array<developed_scalar_film_t<Dims>,4>;

class film_storage_handle_t;

}
