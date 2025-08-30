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

namespace wt {

using mp_units::Quantity;
using mp_units::QuantityOf;
using mp_units::QuantityPoint;
using mp_units::QuantityPointOf;
using mp_units::QuantitySpec;

template<typename T>
concept QuantityRef = mp_units::Reference<T>;
template<typename T, auto Q>
concept QuantityRefOf = mp_units::ReferenceOf<T, Q>;

using mp_units::Dimension;
using mp_units::DimensionOf;

using mp_units::Unit;
using mp_units::UnitOf;

}
