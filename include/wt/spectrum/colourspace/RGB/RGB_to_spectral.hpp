/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/common.hpp>

namespace wt::colourspace::RGB_to_spectral {

inline constexpr auto min_lambda = 380 * u::nm;
inline constexpr auto max_lambda = 720 * u::nm;

/**
 * @brief Uplifts an RGB triplet to spectral. Designed for the optical spectrum.
 *        by Andrea Weidlich.
 * 
 * @param rgb RGB triplet
 * @param lambda wavelength
 * @return f_t intensity
 */
inline f_t uplift(const vec3_t& rgb, const Wavelength auto lambda) noexcept {
    if (lambda<min_lambda || lambda>max_lambda)
        return 0;

    const f_t white_I[11] = { 1.0000, 1.0000, 0.9999, 0.9993, 0.9992, 0.9998, 1.0000, 1.0000, 1.0000, 1.0000, 0.0 };
    const f_t cyan_I[11] = { 0.9710, 0.9426, 1.0007, 1.0007, 1.0007, 1.0007, 0.1564, 0.0000, 0.0000, 0.0000, 0.0 };
    const f_t magenta_I[11] = { 1.0000, 1.0000, 0.968,  0.22295, 0.0000, 0.0458, 0.8369, 1.0000, 1.0000, 0.9959, 0.0 };
    const f_t yellow_I[11] = { 0.0001, 0.0000, 0.1088, 0.6651, 1.0000, 1.0000, 0.9996, 0.9586, 0.9685, 0.9840, 0.0 };
    const f_t red_I[11] = { 0.1012, 0.0515, 0.0000, 0.0000, 0.0000, 0.0000, 0.8325, 1.0149, 1.0149, 1.014, 0.0  };
    const f_t green_I[11] = { 0.0000, 0.0000, 0.0273, 0.7937, 1.0000, 0.9418, 0.1719, 0.0000, 0.0000, 0.0025, 0.0 };
    const f_t blue_I[11] = { 1.0000, 1.0000, 0.8916, 0.3323, 0.0000, 0.0000, 0.0003, 0.0369, 0.0483, 0.0496, 0.0 };

    f_t I = 0;

    const int b = int((lambda-min_lambda) / (max_lambda-min_lambda) * f_t(10));
    const f_t white_s   = white_I[b];
    const f_t cyan_s    = cyan_I[b];
    const f_t magenta_s = magenta_I[b];
    const f_t yellow_s  = yellow_I[b];
    const f_t red_s     = red_I[b];
    const f_t green_s   = green_I[b];
    const f_t blue_s    = blue_I[b];

    if (rgb.x <= rgb.y && rgb.x <= rgb.z) {
        I += white_s * rgb.x;
        if (rgb.y <= rgb.z) {
            I += cyan_s * (rgb.y - rgb.x);
            I += blue_s * (rgb.z - rgb.y);
        }
        else {
            I += cyan_s * (rgb.z - rgb.x);
            I += green_s * (rgb.y - rgb.z);
        }
    }
    else if (rgb.y <= rgb.x && rgb.y <= rgb.z) {
        I += white_s * rgb.y;
        if (rgb.x <= rgb.z) {
            I += magenta_s * (rgb.x - rgb.y);
            I += blue_s * (rgb.z - rgb.x);
        }
        else {
            I += magenta_s * (rgb.z - rgb.y);
            I += red_s * (rgb.x - rgb.z);
        }
    }
    else {
        I += white_s * rgb.z;
        if (rgb.x <= rgb.y) {
            I += yellow_s * (rgb.x - rgb.z);
            I += green_s * (rgb.y - rgb.x);
        }
        else {
            I += yellow_s * (rgb.y - rgb.z);
            I += red_s * (rgb.x - rgb.y);
        }
    }

    return I;
}

}
