/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/quantity/framework.hpp>
#include <wt/math/defs.hpp>
#include <wt/util/concepts.hpp>
#include <mp-units/systems/isq/electromagnetism.h>

namespace wt {

/** @brief ISQ quantities and concepts.
 */
namespace isq {
using namespace mp_units::isq;
/**
 * @brief EM power.
 */
inline constexpr auto power = mp_units::isq::electromagnetism_power;
}

/** @brief SI constants.
 */
namespace siconstants {
using mp_units::si::standard_gravity;
using mp_units::si::magnetic_constant;
using namespace mp_units::si::si2019;
}

template <mp_units::Reference auto R, Numeric Rep=f_t>
using quantity = mp_units::quantity<R, Rep>;
template <mp_units::Reference auto R, mp_units::PointOrigin auto O, Numeric Rep=f_t>
using quantity_point = mp_units::quantity_point<R, O, Rep>;

using mp_units::dimensionless;
using mp_units::delta;
using mp_units::point;

using mp_units::inverse;
using mp_units::square;
using mp_units::pow;

namespace u {

using mp_units::one;
using namespace mp_units::si::unit_symbols;

namespace ang {
using namespace mp_units::angular::unit_symbols;
}

}

/** @brief angular quantity specs.
 */
namespace angular {
using mp_units::angular::angle;
using mp_units::angular::solid_angle;
}

// EM quantity specs
namespace electrodynamics {

/**
 * @brief EM radiation wavelength.
 */
inline constexpr auto wavelength = isq::length;
/**
 * @brief EM radiation wavenumber.
 */
inline constexpr auto wavenumber = inverse(isq::length);

/**
 * @brief EM radiation frequency,
 */
inline constexpr auto frequency = inverse(isq::time);

/** @brief radiometric power.
 */
inline constexpr struct radiant_flux final : ::mp_units::quantity_spec<radiant_flux, isq::power> {} radiant_flux;
/** @brief quantum efficiency.
 */
inline constexpr struct QE final : ::mp_units::quantity_spec<QE, dimensionless> {} QE;

/** @brief radiant intensity (radiometric power per solid angle).
 */
inline constexpr struct radiant_intensity final :
        ::mp_units::quantity_spec<radiant_intensity, radiant_flux / angular::solid_angle> {}
radiant_intensity;
/** @brief quantum efficiency times solid angle.
 */
inline constexpr struct QE_solid_angle final :
        ::mp_units::quantity_spec<QE_solid_angle, QE * angular::solid_angle> {} QE_solid_angle;

/** @brief irradiance (radiometric power per area).
 */
inline constexpr struct irradiance final :
        ::mp_units::quantity_spec<irradiance, radiant_flux / isq::area> {} irradiance;
/** @brief quantum efficiency times area.
 */
inline constexpr struct QE_area final :
        ::mp_units::quantity_spec<QE_area, QE * isq::area> {} QE_area;

/** @brief radiance (radiometric power per solid angle per area).
 */
inline constexpr struct radiance final :
        ::mp_units::quantity_spec<radiance, radiant_flux / angular::solid_angle / isq::area> {}
radiance;
/** @brief quantum efficiency times solid angle times area.
 */
inline constexpr struct QE_solid_angle_area final :
        ::mp_units::quantity_spec<QE_solid_angle_area, QE * angular::solid_angle * isq::area> {} QE_solid_angle_area;

}


/* common spatial quantities
 */

/** @brief spatial length.
 */
using length_t = quantity<isq::length[u::m]>;
/** @brief density per spatial length.
 */
using length_density_t = quantity<inverse(length_t::reference)>;

/** @brief spatial area.
 */
using area_t = quantity<isq::area[square(u::m)]>;
/** @brief density per spatial area.
 */
using area_density_t = quantity<inverse(area_t::reference)>;

/** @brief spatial volume.
 */
using volume_t = quantity<isq::volume[pow<3>(u::m)]>;
/** @brief density per spatial volume.
 */
using volume_density_t = quantity<inverse(volume_t::reference)>;


template<typename T>
concept Length = QuantityOf<T, isq::length>;
template<typename T>
concept Area = QuantityOf<T, isq::area>;
template<typename T>
concept Volume = QuantityOf<T, isq::volume>;


/* common angular quantities
 */

/** @brief angle.
 */
using angle_t               = quantity<angular::angle[u::ang::rad]>;
/** @brief density per angle.
 */
using angle_density_t       = quantity<inverse(angle_t::reference)>;
/** @brief solid angle.
 */
using solid_angle_t         = quantity<angular::solid_angle[u::ang::sr]>;
/** @brief density per solid angle.
 */
using solid_angle_density_t = quantity<inverse(solid_angle_t::reference)>;


template<typename T>
concept Angle = QuantityOf<T, angle_t::quantity_spec>;
template<typename T>
concept SolidAngle = QuantityOf<T, solid_angle_t::quantity_spec>;


/* light and electrodynamics quantities
 */

/** @brief radiation frequency,
 *        Vacuum wavelength \f$ \lambda \f$ is related to frequency \f$ f \f$ via \f$ \lambda = \frac{c}{f} \f$, where \f$ c \f$ is speed of light (in vacuum).
 */
using frequency_t = quantity<electrodynamics::frequency[u::GHz]>;

/** @brief radiation wavelength.
 */
using wavelength_t = quantity<electrodynamics::wavelength[u::mm]>;
/** @brief density per radiation wavelength.
 */
using wavelength_density_t = quantity<inverse(wavelength_t::reference)>;
/** @brief radiation wavenumber.
 *        Wavenumber \f$ k \f$ is related to wavelength \f$ \lambda \f$ via \f$ k = \frac{2\pi}{\lambda} \f$.
 */
using wavenumber_t = quantity<electrodynamics::wavenumber[u::one / u::mm]>;
/** @brief density per radiation wavenumber.
 */
using wavenumber_density_t = quantity<inverse(wavenumber_t::reference)>;

/** @brief power (Watts).
 */
using power_t = quantity<isq::power[u::W]>;

/** @brief radiometric power (Watts).
 */
using radiant_flux_t = quantity<electrodynamics::radiant_flux[u::W]>;
/** @brief spectral (per wavenumber) radiometric power.
 */
using spectral_radiant_flux_t = quantity<
    (electrodynamics::radiant_flux[u::W]) /
    (electrodynamics::wavenumber[u::one / u::mm])
>;
/** @brief quantum efficiency, also known as importance; (dimensionless).
 */
using QE_t = quantity<electrodynamics::QE[u::one]>;

/** @brief radiant intensity (radiometric power per solid angle).
 */
using radiant_intensity_t = quantity<electrodynamics::radiant_intensity[u::W / solid_angle_t::unit]>;
/** @brief spectral (per wavenumber) radiant intensity (radiometric power per solid angle per wavelength).
 */
using spectral_radiant_intensity_t = quantity<
    (electrodynamics::radiant_intensity[u::W / solid_angle_t::unit]) /
    (electrodynamics::wavenumber[u::one / u::mm])
>;
/** @brief quantum efficiency times area.
 *         i.e., importance flux per solid angle (importance analogue of radiant flux).
 */
using QE_area_t = quantity<electrodynamics::QE_area[area_t::unit]>;

/** @brief irradiance (radiometric power per area).
 */
using irradiance_t = quantity<electrodynamics::irradiance[u::W / area_t::unit]>;
/** @brief spectral (per wavenumber) irradiance (radiometric power per solid angle area).
 */
using spectral_irradiance_t = quantity<
    (electrodynamics::irradiance[u::W / area_t::unit]) /
    (electrodynamics::wavenumber[u::one / u::mm])
>;
/** @brief quantum efficiency times solid angle.
 *         i.e., importance flux per area (importance analogue of irradiance).
 */
using QE_solid_angle_t = quantity<electrodynamics::QE_solid_angle[solid_angle_t::unit]>;

/** @brief radiance (radiometric power per solid angle per area).
 */
using radiance_t = quantity<electrodynamics::radiance[u::W / solid_angle_t::unit / area_t::unit]>;
/** @brief spectral (per wavenumber) radiance (radiometric power per solid angle per area per wavelength).
 */
using spectral_radiance_t = quantity<
    (electrodynamics::radiance[u::W / solid_angle_t::unit / area_t::unit]) /
    (electrodynamics::wavenumber[u::one / u::mm])
>;
/** @brief quantum efficiency of radiance (quantum efficiency per solid angle per area).
 */
using QE_flux_t = quantity<
    electrodynamics::QE_solid_angle_area[solid_angle_t::unit * area_t::unit]
>;


template<typename T>
concept Frequency = QuantityOf<T, frequency_t::quantity_spec>;
template<typename T>
concept Wavelength = QuantityOf<T, wavelength_t::quantity_spec>;
template<typename T>
concept Wavenumber = QuantityOf<T, wavenumber_t::quantity_spec>;


/* common thermodynamic quantities
 */

/** @brief thermodynamic temperature.
 */
using temperature_t = quantity_point<
    isq::thermodynamic_temperature[u::K], 
    mp_units::default_point_origin(isq::thermodynamic_temperature[u::K]), 
    f_t
>;


template<typename T>
concept Temperature = QuantityPointOf<T, temperature_t::quantity_spec>;


/* shorthands for quantity to value conversions
 */

namespace u {

/**
 * @brief Converts a dimensionless quantity to its underlying representation.
 */
constexpr inline auto to_num(const QuantityOf<dimensionless> auto q) noexcept {
    return q.numerical_value_in(u::one);
}

/**
 * @brief Converts a length quantity to its underlying representation in nanometres.
 */
constexpr inline auto to_nm(const QuantityOf<isq::length> auto q) noexcept {
    return q.numerical_value_in(u::nm);
}

/**
 * @brief Converts a length quantity to its underlying representation in millimetres.
 */
constexpr inline auto to_mm(const QuantityOf<isq::length> auto q) noexcept {
    return q.numerical_value_in(u::mm);
}

/**
 * @brief Converts a length quantity to its underlying representation in micrometres.
 */
constexpr inline auto to_µm(const QuantityOf<isq::length> auto q) noexcept {
    return q.numerical_value_in(u::µm);
}

/**
 * @brief Converts a length quantity to its underlying representation in metres.
 */
constexpr inline auto to_m(const QuantityOf<isq::length> auto q) noexcept {
    return q.numerical_value_in(u::m);
}

/**
 * @brief Converts an area quantity to its underlying representation in millimetres^2.
 */
constexpr inline auto to_mm2(const QuantityOf<isq::area> auto q) noexcept {
    return q.numerical_value_in(square(u::mm));
}

/**
 * @brief Converts an area quantity to its underlying representation in metres^2.
 */
constexpr inline auto to_m2(const QuantityOf<isq::area> auto q) noexcept {
    return q.numerical_value_in(area_t::unit);
}

/**
 * @brief Converts an inverse length quantity to its underlying representation in m^(-1).
 */
constexpr inline auto to_inv_m(const QuantityOf<inverse(isq::length)> auto q) noexcept {
    return q.numerical_value_in(u::one/u::m);
}

/**
 * @brief Converts an inverse length quantity to its underlying representation in mm^(-1).
 */
constexpr inline auto to_inv_mm(const QuantityOf<inverse(isq::length)> auto q) noexcept {
    return q.numerical_value_in(u::one / u::mm);
}

/**
 * @brief Converts an inverse length quantity to its underlying representation in m^(-2).
 */
constexpr inline auto to_inv_m2(const QuantityOf<inverse(isq::area)> auto q) noexcept {
    return q.numerical_value_in(inverse(area_t::unit));
}

/**
 * @brief Converts an inverse length quantity to its underlying representation in mm^(-2).
 */
constexpr inline auto to_inv_mm2(const QuantityOf<inverse(isq::area)> auto q) noexcept {
    return q.numerical_value_in(inverse(square(u::mm)));
}

/**
 * @brief Converts an angle quantity to its underlying representation in radians.
 */
constexpr inline auto to_rad(const QuantityOf<angular::angle> auto q) noexcept {
    return q.numerical_value_in(u::ang::rad);
}

/**
 * @brief Converts a solid angle quantity to its underlying representation in steradians.
 */
constexpr inline auto to_sr(const QuantityOf<angular::solid_angle> auto q) noexcept {
    return q.numerical_value_in(solid_angle_t::unit);
}

/**
 * @brief Converts a frequency quantity to its underlying representation in Hz.
 */
constexpr inline auto to_Hz(const QuantityOf<electrodynamics::frequency> auto q) noexcept {
    return q.numerical_value_in(u::Hz);
}

/**
 * @brief Converts a frequency quantity to its underlying representation in kHz.
 */
constexpr inline auto to_kHz(const QuantityOf<electrodynamics::frequency> auto q) noexcept {
    return q.numerical_value_in(u::kHz);
}

/**
 * @brief Converts a frequency quantity to its underlying representation in MHz.
 */
constexpr inline auto to_MHz(const QuantityOf<electrodynamics::frequency> auto q) noexcept {
    return q.numerical_value_in(u::MHz);
}

/**
 * @brief Converts a frequency quantity to its underlying representation in GHz.
 */
constexpr inline auto to_GHz(const QuantityOf<electrodynamics::frequency> auto q) noexcept {
    return q.numerical_value_in(u::GHz);
}

/**
 * @brief Converts a power quantity to its underlying representation in Watt.
 */
constexpr inline auto to_W(const QuantityOf<isq::power> auto q) noexcept {
    return q.numerical_value_in(u::W);
}

template <Unit U>
constexpr inline auto numerical_value_in(const Quantity auto& q, const U u) noexcept {
    return q.numerical_value_in(u);
}

}

}
