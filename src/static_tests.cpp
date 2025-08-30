/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <type_traits>

#include <wt/math/common.hpp>

using namespace wt;


static_assert(std::is_same_v<f_t,float> ||
              std::is_same_v<f_t,double> ||
              std::is_same_v<f_t,long double>);
