/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/


#include <array>
#include <map>
#include <functional>

#include <wt/spectrum/util/spectrum_from_ITU.hpp>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>

#include <wt/spectrum/complex_analytic.hpp>

using namespace wt;
using namespace spectrum;


struct ITU_params_t {
    f_t a,b,c,d;
    range_t<wavenumber_t> range;
};

template <std::size_t N>
inline constexpr c_t IOR_for_ITU(const std::array<ITU_params_t, N>& params, const Wavenumber auto k) noexcept {
    const auto freq = (frequency_t)wavenum_to_freq(k);
    const auto f_ghz = (f_t)u::to_GHz(freq);
    [[assume(f_ghz>0)]];

    constexpr auto permeability_of_vacuum   = f_t(1) * siconstants::magnetic_constant;
    constexpr auto speed_of_light_in_vacuum = f_t(1) * siconstants::speed_of_light_in_vacuum;

    for (const auto& p : params) {
        if (p.range.contains(k)) {
            const auto rel_permittivity = p.a * (p.b!=0 ? m::pow(f_ghz, p.b) : 1);
            const auto conductivity     = p.c * (p.d!=0 ? m::pow(f_ghz, p.d) : 1) * u::S/u::m;

            const auto temporal_freq = k * speed_of_light_in_vacuum;
            const auto epsilon0 = 1 / (permeability_of_vacuum * pow<2>(speed_of_light_in_vacuum));
            const auto rel_sigma = u::to_num(-conductivity / (epsilon0 * temporal_freq));

            return std::sqrt(c_t{ rel_permittivity, rel_sigma });
        }
    }
    return c_t{ 0 };
}

using ITU_complex_spectrum_generator_t = std::function<std::unique_ptr<complex_analytic_t>(std::string)>;

template <std::size_t N>
inline ITU_complex_spectrum_generator_t spectrum_for_ITU(
        const std::array<ITU_params_t, N>& params, const std::string& description) noexcept {
    auto range = range_t<wavenumber_t>::null();
    for (const auto& p : params)
        range |= p.range;

    return [params,range,description](std::string id) {
        return std::make_unique<complex_analytic_t>(
            std::move(id), range,
            [params](const wavenumber_t k) noexcept { return IOR_for_ITU(params, k); },
            description
        );
    };
}

static const std::map<std::string, ITU_complex_spectrum_generator_t> ITU_P2040_2_table3 = {
    { std::string{ "vacuum" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 1,    0,    0,         0,      
                            { 0/u::mm, limits<wavenumber_t>::infinity() } }
        },
        "ITU-vacuum"
    ) },
    { std::string{ "concrete" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 5.24, 0,    0.0462,    0.7822, 
                            { freq_to_wavenum(1 * u::GHz),     freq_to_wavenum(100 * u::GHz) } }
        },
        "ITU-concrete"
    ) },
    { std::string{ "brick" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 3.91, 0,    0.0238,    0.16,   
                            { freq_to_wavenum(1 * u::GHz),     freq_to_wavenum(40 * u::GHz)  } }
        },
        "ITU-brick"
    ) },
    { std::string{ "plasterboard" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 2.73, 0,    0.0085,    0.9395, 
                            { freq_to_wavenum(1 * u::GHz),     freq_to_wavenum(100 * u::GHz) } }
        },
        "ITU-plasterboard"
    ) },
    { std::string{ "wood" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 1.99, 0,    0.0047,    1.0718, 
                            { freq_to_wavenum(0.001 * u::GHz), freq_to_wavenum(100 * u::GHz) } }
        },
        "ITU-wood"
    ) },
    { std::string{ "glass" }, spectrum_for_ITU<2>( {
            ITU_params_t{ 6.31, 0,    0.0036,    1.3394, 
                            { freq_to_wavenum(0.1 * u::GHz),   freq_to_wavenum(100 * u::GHz) } },
            ITU_params_t{ 5.79, 0,    0.0004,    1.658,  
                            { freq_to_wavenum(220 * u::GHz),   freq_to_wavenum(450 * u::GHz) } }
        },
        "ITU-glass"
    ) },
    { std::string{ "ceiling_board" }, spectrum_for_ITU<2>({ 
            ITU_params_t{ 1.48, 0,    0.0011,    1.0750, 
                            { freq_to_wavenum(1 * u::GHz),     freq_to_wavenum(100 * u::GHz) } },
            ITU_params_t{ 1.52, 0,    0.0029,    1.029,  
                            { freq_to_wavenum(220 * u::GHz),   freq_to_wavenum(450 * u::GHz) } }
        },
        "ITU-ceiling_board"
    ) },
    { std::string{ "chipboard" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 2.58, 0,    0.0217,    0.7800, 
                            { freq_to_wavenum(1 * u::GHz),     freq_to_wavenum(100 * u::GHz) } }
        },
        "ITU-chipboard"
    ) },
    { std::string{ "plywood" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 2.71, 0,    0.33,      0,      
                            { freq_to_wavenum(1 * u::GHz),     freq_to_wavenum(40 * u::GHz)  } }
        },
        "ITU-plywood"
    ) },
    { std::string{ "marble" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 7.074,0,    0.0055,    0.9262, 
                            { freq_to_wavenum(1 * u::GHz),     freq_to_wavenum(60 * u::GHz)  } }
        },
        "ITU-marble"
    ) },
    { std::string{ "floorboard" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 3.66, 0,    0.0044,    1.3515, 
                            { freq_to_wavenum(50 * u::GHz),    freq_to_wavenum(100 * u::GHz) } }
        },
        "ITU-floorboard"
    ) },
    { std::string{ "metal" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 1,    0,    f_t(1e+7), 0,      
                            { freq_to_wavenum(1 * u::GHz),     freq_to_wavenum(100 * u::GHz) } }
        },
        "ITU-metal"
    ) },
    { std::string{ "very_dry_ground" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 3,    0,    0.00015,   2.52,   
                            { freq_to_wavenum(1 * u::GHz),     freq_to_wavenum(10 * u::GHz)  } }
        },
        "ITU-very_dry_ground"
    ) },
    { std::string{ "medium_dry_ground" },spectrum_for_ITU<1>( {
            ITU_params_t{ 15,   -0.1, 0.035,     1.63,   
                            { freq_to_wavenum(1 * u::GHz),     freq_to_wavenum(10 * u::GHz)  } }
        },
        "ITU-medium_dry_ground"
    ) },
    { std::string{ "wet_ground" }, spectrum_for_ITU<1>({ 
            ITU_params_t{ 30,   -0.4, 0.15,      1.30,   
                            { freq_to_wavenum(1 * u::GHz),     freq_to_wavenum(10 * u::GHz)  } } 
        },
        "ITU-wet_ground"
    ) },
};

std::unique_ptr<spectrum_t> wt::spectrum::spectrum_from_ITU_material(std::string id, const std::string& material) {
    const auto it = ITU_P2040_2_table3.find(material);
    if (it!=ITU_P2040_2_table3.end()) {
        return it->second(std::move(id));
    }

    return nullptr;
}
