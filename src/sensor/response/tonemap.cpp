/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/wt_context.hpp>
#include <wt/scene/loader/node.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/sensor/response/tonemap/tonemap.hpp>
#include <wt/spectrum/colourspace/RGB/RGB.hpp>

#include <wt/math/common.hpp>
#include <wt/scene/loader/node_readers.hpp>
#include <wt/util/unreachable.hpp>
#include <wt/util/logger/logger.hpp>

#include <utility>
#include <optional>

using namespace wt;
using namespace sensor::response;


tonemap_t::tonemap_t(
        std::string id,
        tonemap_mode_e mode,
        tonemap_e func,
        tinycolormap::ColormapType colourmap,
        std::optional<compiled_math_expression_t> user_func,
        const f_t gamma,
        const range_t<>& db_range)
    : scene_element_t(std::move(id)),
      mode(mode),
      func(func),
      user_func(std::move(user_func)),
      colourmap_name(std::format("{}", colourmap))
{
    if (func == tonemap_e::gamma)
        this->gamma = gamma;
    else if (func == tonemap_e::dB)
        this->db_range = db_range;

    // build evaluation functions
    std::function<f_t(f_t)> apply_func;
    switch (func) {
    case tonemap_e::linear:    apply_func = [](f_t value) { return value; };
        break;
    case tonemap_e::function:  apply_func = [this](f_t value) { return eval_user_func(value); };
        break;
    case tonemap_e::gamma:     apply_func = [rg=1/gamma](f_t value) { return m::pow(m::clamp01(value),rg); };
        break;
    case tonemap_e::sRGB:      apply_func = [](f_t value) { return colourspace::sRGB::from_linear(m::clamp01(value)); };
        break;
    case tonemap_e::dB:
        apply_func = [r=db_range](f_t value) -> f_t {
            if (value==0)
                return 0;

            const auto db = 10/std::numbers::ln10_v<f_t> * m::log(value);
            return m::clamp01((db-r.min)/r.length());
        };
        break;
    default:
        unreachable();
    }

    unique_function<vec3_t(f_t) const noexcept> function_cm_mono = [apply_func, colourmap](f_t value) noexcept {
            value = apply_func(value);
            return vec3_t{ tinycolormap::GetColor(value, colourmap).ConvertToGLM() };
        };
    unique_function<vec3_t(vec3_t) const noexcept> function_cm_poly = [apply_func, colourmap](vec3_t value) noexcept {
            const auto v = apply_func(colourspace::luminance(value));
            return vec3_t{ tinycolormap::GetColor(v, colourmap).ConvertToGLM() };
        };

    unique_function<vec3_t(f_t) const noexcept> function_norm_mono = [apply_func](f_t value) noexcept {
            return vec3_t{ apply_func(value) };
        };
    unique_function<vec3_t(vec3_t) const noexcept> function_norm_poly = [apply_func](vec3_t value) noexcept {
            value[0] = apply_func(value[0]);
            value[1] = apply_func(value[1]);
            value[2] = apply_func(value[2]);
            return value;
        };

    switch (mode) {
    case tonemap_mode_e::colourmap:
        this->function_mono = std::move(function_cm_mono);
        this->function_poly = std::move(function_cm_poly);
        break;
    case tonemap_mode_e::normal:
        this->function_mono = std::move(function_norm_mono);
        this->function_poly = std::move(function_norm_poly);
        break;
    case tonemap_mode_e::select:
        this->function_mono = std::move(function_cm_mono);
        this->function_poly = std::move(function_norm_poly);
        break;
    default:
        unreachable();
    }
}


scene::element::info_t tonemap_t::description() const {
    using namespace scene::element;

    auto ret = info_for_scene_element(*this, "tonemap", {
        { "function", attributes::make_enum(func) },
        { "mode",     attributes::make_enum(mode) },
    });

    if (func == tonemap_e::gamma)
        ret.attribs.emplace("gamma", attributes::make_scalar(gamma));
    if (func == tonemap_e::dB)
        ret.attribs.emplace("dB", attributes::make_range(db_range));

    return ret;
}


std::unique_ptr<tonemap_t> tonemap_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    const auto& type = node["type"];
    tonemap_e tonemap;
         if (type=="linear")   tonemap = tonemap_e::linear;
    else if (type=="gamma")    tonemap = tonemap_e::gamma;
    else if (type=="sRGB")     tonemap = tonemap_e::sRGB;
    else if (type=="dB")       tonemap = tonemap_e::dB;
    else if (type=="function") tonemap = tonemap_e::function;
    else
        throw std::runtime_error("(tonemap operator loader) Unrecognized 'type'");

    auto mode = tonemap_mode_e::select;
    auto colourmap = default_colourmap;

    std::optional<range_t<>> db_range;
    f_t gamma = 2.2;
    std::optional<compiled_math_expression_t> user_function;

    try {
    for (auto& item : node.children_view()) {
        if (!scene::loader::read_enum_attribute(item, "mode", mode) &&
            !scene::loader::read_enum_attribute(item, "colourmap", colourmap) &&
            !scene::loader::read_attribute(item, db_range) &&
            !scene::loader::load_function(item, user_function, { "value" }) &&
            !scene::loader::read_attribute(item, "gamma", gamma))
            logger::cwarn()
                << loader->node_description(item)
                << "(tonemap operator loader) Unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    }
    } catch(const std::format_error& exp) {
        throw std::runtime_error("(tonemap operator loader) " + std::string{ exp.what() });
    }

    if (tonemap==tonemap_e::function && !user_function)
        throw scene_loading_exception_t("(tonemap operator loader) expected 'function' to be provided", node);
    if (tonemap==tonemap_e::dB && (!db_range || db_range->length()<=0))
        throw scene_loading_exception_t("(tonemap operator loader) expected valid 'db' range to be provided", node);
    if (tonemap==tonemap_e::gamma && gamma<=0)
        throw scene_loading_exception_t("(tonemap operator loader) 'gamma' must be positive", node);
    
    return std::make_unique<tonemap_t>(
        std::move(id), mode, tonemap, colourmap,
        std::move(user_function),
        gamma, db_range.value_or(range_t<>{ 0,0 })
    );
}
