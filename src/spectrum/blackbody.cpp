/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <algorithm>
#include <string>
#include <vector>

#include <wt/scene/loader/node.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/spectrum/blackbody.hpp>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>

#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::spectrum;


inline auto piecewise_linear_blackbody_approximation(const std::string& id, 
                                                     const temperature_t T, 
                                                     range_t<wavelength_t> wl_range,
                                                     const f_t scale) noexcept {
    // start with 8nm steps, and increase after 800nm to 8+lambda/100 step.
    constexpr auto wl_step0 = 8. * u::nm;
    constexpr auto wl_step1 = 800. * u::nm;
    constexpr auto wl_step1_factor = 1./100.;

    std::vector<vec2_t> values;
    for (auto lambda = m::max<quantity<u::nm,double>>(wl_step0, wl_range.min-wl_step0); 
         lambda<=wl_range.max+wl_step0;) {
        const auto b = colourspace::planck_blackbody(lambda, T);
        const auto k = wavelen_to_wavenum(lambda);
        values.emplace_back(u::to_inv_mm(k), b * scale);

        assert(m::isfinite(lambda) && m::isfinite(k) && m::isfinite(b));

        const auto wl_step = lambda < wl_step1 ? wl_step0 : 
                             wl_step0 + lambda*wl_step1_factor;
        lambda += wl_step;
    }

    std::ranges::reverse(values);

    return piecewise_linear_t{ id+"_blackbody_piecewiselinear", piecewise_linear_distribution_t{ std::move(values) } };
}


blackbody_t::blackbody_t(std::string id, const temperature_t T, const range_t<wavelength_t>& wl_range, const f_t scale)
    : spectrum_real_t(std::move(id)),
      spectrum(piecewise_linear_blackbody_approximation(get_id(), T, wl_range, scale)),
      T(T)
{}

scene::element::info_t blackbody_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "blackbody", {
        { "temperature",           attributes::make_scalar(temperature()) },
        { "Planckian locus (XYZ)", attributes::make_vector(locus_XYZ()) },
    });
}


std::unique_ptr<spectrum_real_t> blackbody_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    const auto T = stoq_strict<temperature_t>(node["blackbody"]);
    f_t scale = 1;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::read_attribute(item, "scale", scale))
            logger::cwarn()
                << loader->node_description(item)
                << "(blackbody spectrum loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(blackbody spectrum loader) " + std::string{ exp.what() }, item);
    }
    }

    // wavelength range of blackbody spectrum, needed for sensors
    // hardcoded max & min
    auto range = range_t<wavelength_t>::range(
        10 * u::nm,
        5 * u::mm
    );

    if (scale<0)
        throw scene_loading_exception_t("(blackbody spectrum loader) 'scale' must be non-negative", node);
    if (T.quantity_from_zero()<=zero)
        throw scene_loading_exception_t("(blackbody spectrum loader) temperature must be provided and be positive (in Kelvin)", node);

    return std::make_unique<blackbody_t>(std::move(id), T, range, scale);
}
