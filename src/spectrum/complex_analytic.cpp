/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>

#include <wt/scene/element/attributes.hpp>
#include <wt/spectrum/complex_analytic.hpp>

#include <wt/math/common.hpp>

using namespace wt;
using namespace wt::spectrum;


scene::element::info_t complex_analytic_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "complex_analytic", {
        { "function", attributes::make_string(this->func_description) },
    });
}
