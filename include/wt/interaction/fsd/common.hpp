/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <vector>
#include <optional>

#include <wt/ads/common.hpp>
#include <wt/math/common.hpp>

namespace wt::utd {

struct UTD_ret_t {
    /**
     * @brief Diffraction coefficients.
     */
    c_t Ds, Dh;

    /**
     * @brief SH incident frame.
     */
    dir3_t si,hi;
    /**
     * @brief SH scattering frame.
     */
    dir3_t so,ho;
};

/**
 * @brief Wedge edge
 */
struct wedge_edge_t {
    /** @brief Edge mid point.
     */
    pqvec3_t v;
    /** @brief Edge length.
     */
    length_t l;
    /** @brief Front-face normal and tangent direction pointing into the wedge.
     */
    dir3_t nff, tff;
    /** @brief Back-face normal.
     */
    dir3_t nbf;

    /** @brief Wedge opening angle.
     */
    angle_t alpha;
    /** @brief Refractive-index.
     */
    f_t eta;

    ads::tuid_t ads_edge_idx;

    /**
    * @brief Edge direction.
    */
    [[nodiscard]] inline auto e() const noexcept {
        return dir3_t{ m::cross(nff,tff) };
    }

    /**
    * @brief Returns point on wedge that satisfies Fermat's principle, if such a point exists.
    */
    [[nodiscard]] std::optional<pqvec3_t> diffraction_point(
            const pqvec3_t& src,
            const pqvec3_t& dst) const noexcept;

    /**
    * @brief Returns point on wedge that satisfies Fermat's principle, if such a point exists.
    */
    [[nodiscard]] std::optional<pqvec3_t> diffraction_point(
            const pqvec3_t& src,
            const dir3_t& wo) const noexcept;

    /**
    * @brief The UTD wedge diffraction function. Does NOT account for the phase term exp(-i*k*ro).
    */
    [[nodiscard]] UTD_ret_t UTD(const Wavenumber auto k,
                                const dir3_t& wi,
                                const dir3_t& wo,
                                const length_t ro) const noexcept;
};

/**
 * @brief An FSD aperture
 */
struct fsd_aperture_t {
    std::vector<wedge_edge_t> edges;  // wedges composing the aperture
    wavenumber_t k;

    [[nodiscard]] inline bool single_edge() const noexcept { return edges.size()==1; }
};

}
