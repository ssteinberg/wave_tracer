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
#include <vector>
#include <memory>

#include <wt/wt_context.hpp>
#include <wt/spectrum/spectrum.hpp>

namespace wt::spectrum {

namespace detail_spectrum_from_db {

struct spectrum_from_material_ret_t {
    std::string id;
    std::vector<std::unique_ptr<spectrum_t>> channels;
};

/**
 * @brief Loads a spectrum (or several channels of spectra) from database file.
 * @param bin Should convert the piecewise-linear spectrum to a binned (equal-spaced) spectrum?
 */
spectrum_from_material_ret_t spectrum_from_db(const wt_context_t& ctx,
                                              const std::string& name,
                                              const std::uint16_t channels,
                                              const wavelength_t db_wavelength,
                                              const f_t scale,
                                              const bool bin = false);

}

/**
 * @brief Loads an emission spectrum from database file "emission/{name}".
 */
inline std::unique_ptr<spectrum_t> emission_spectrum_from_db(
        const wt_context_t &ctx,
        const std::string& name,
        const f_t scale=1) {
    // emission db are provided with wavelength in nm
    auto ret = detail_spectrum_from_db::spectrum_from_db(ctx, "emission/"+name, 1,
                                                         1*u::nm, scale);
    assert(!ret.channels.empty());
    return ret.channels.empty() ? nullptr : std::move(ret.channels[0]);
}

/**
 * @brief Loads a material IOR (may be real or complex) spectrum from database file "ior/{name}".
 */
inline auto spectrum_from_material(const wt_context_t& ctx,
                                   const std::string& name) {
    // IOR db are provided with wavelength in um
    return detail_spectrum_from_db::spectrum_from_db(ctx, "ior/"+name, 2,
                                                     1*u::µm, 1);
}

/**
 * @brief Loads a response (QE) spectrum from database file "sensitivity/{name}".
 * @param bin Should convert the piecewise-linear spectrum to a binned (equal-spaced) spectrum?
 */
inline auto response_spectrum_from_db(const wt_context_t& ctx,
                                      const std::string& name,
                                      const std::uint16_t channels,
                                      const bool bin = true) {
    // sensitivity db are provided with wavelength in nm
    return detail_spectrum_from_db::spectrum_from_db(ctx, "sensitivity/"+name, channels,
                                                     1*u::nm, 1,
                                                     bin);
}

}
