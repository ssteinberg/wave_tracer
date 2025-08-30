/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <memory>

#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

#include <wt/util/array.hpp>

namespace wt {

class scene_loader;
    
namespace fraunhofer::fsd_sampler {

class fsd_lut_t {
private:
    static constexpr std::size_t Nsamples = 2048, Msamples = 3072;

    struct data_t {
        wt::array_t<f_t, Nsamples> iCDFtheta1, iCDFtheta2;
        wt::array_t<f_t, Msamples,Msamples> iCDF1, iCDF2;
    };
    std::unique_ptr<data_t> data;

    // linear interpolation of LUTs
    template <std::size_t S>
    static inline auto lerp(f_t x, const wt::array_t<f_t, S> &tbl) {
        x *= S-1;
        const auto l = m::min((std::size_t)x, S-1);
        const auto h = m::min(l+1, S-1);
        const auto f = m::fract(x);
        return f*tbl[h] + (1-f)*tbl[l];
    }
    // bilinear interpolation of LUTs
    template <std::size_t Sx, std::size_t Sy>
    static inline auto lerp(f_t x, f_t rx, const wt::array_t<f_t, Sx,Sy> &tbl) {
        x *= Sy-1;
        const auto l = std::min((std::size_t)x, Sx-1);
        const auto h = std::min(l+1, Sx-1);
        const auto f = m::fract(x);
        return f*lerp<Sx>(rx,tbl[h]) + (1-f)*lerp<Sx>(rx,tbl[l]);
    }
    
    static inline vec2_t sample(
            const vec3_t &rand3, 
            const wt::array_t<f_t, Nsamples> &iCDFtheta,
            const wt::array_t<f_t, Msamples,Msamples> &iCDF) noexcept {
        const auto theta = lerp<Nsamples>(rand3.x, iCDFtheta) * u::ang::rad;
        const auto theta_fract = (f_t)(theta * 2/(m::pi*u::ang::rad));
        const auto r = m::max((f_t)0, lerp<Msamples,Msamples>(theta_fract, rand3.y, iCDF));

        auto zeta = r * vec2_t{ m::cos(theta), m::sin(theta) };
        // random quadrant
        const auto q = m::min(3,(int)(rand3.z*4));
        zeta.x *= f_t(((q+1)/2)%2==0?1:-1);
        zeta.y *= f_t((q/2)%2==0?1:-1);

        return zeta;
    }

public:
    fsd_lut_t(const wt_context_t &context);

    /**
     * @brief Samples the |𝛂1|² function. 
     */
    [[nodiscard]] inline auto sample_A1(const vec3_t &rand3) const noexcept {
        return sample(rand3, data->iCDFtheta1, data->iCDF1);
    }
    /**
     * @brief Samples the |𝛂2|² function. 
     */
    [[nodiscard]] inline auto sample_A2(const vec3_t &rand3) const noexcept {
        return sample(rand3, data->iCDFtheta2, data->iCDF2);
    }
};

}
}
