/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/quantity/defs.hpp>

namespace wt {

template<typename T>
concept QuantityDerivedFromQE = 
    QuantityOf<T, electrodynamics::QE> ||
    QuantityOf<T, electrodynamics::QE_area> ||
    QuantityOf<T, electrodynamics::QE_solid_angle> ||
    QuantityOf<T, electrodynamics::QE_solid_angle_area>;

template<typename T>
concept QuantityDerivedFromRadiantFlux = 
    QuantityOf<T, electrodynamics::radiant_flux> ||
    QuantityOf<T, electrodynamics::radiant_intensity> ||
    QuantityOf<T, electrodynamics::irradiance> ||
    QuantityOf<T, electrodynamics::radiance>;

}
