/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <vector>

#include <wt/math/common.hpp>
#include <wt/math/norm_integers.hpp>
#include <wt/bitmap/common.hpp>

using namespace wt;
using namespace wt::bitmap;

const colour_encoding_t::srgb_lut_t colour_encoding_t::srgb_lut;

colour_encoding_t::srgb_lut_t::srgb_lut_t() noexcept {
    // 8-bit
    srgb_to_linear_lut8.reserve(256);
    for (int i=0;i<256;++i) {
        const auto f = colourspace::sRGB::to_linear(m::unorm_to_fp(std::uint8_t(i)));
        srgb_to_linear_lut8.emplace_back(f);
    }

    // 16-bit
    srgb_to_linear_lut16.reserve(lut16_count);
    for (int i=0;i<lut16_count;++i) {
        const auto i16 = std::uint16_t(i<<lut16_lsbs);
        const auto f = colourspace::sRGB::to_linear(m::unorm_to_fp(i16));
        srgb_to_linear_lut16.emplace_back(i==lut16_count-1 ? 1 : f);
    }
}
