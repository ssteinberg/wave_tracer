/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <optional>
#include <string>

#include <wt/wt_context.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/sensor/response/response.hpp>
#include <wt/sensor/response/RGB.hpp>
#include <wt/sensor/response/XYZ.hpp>

#include <wt/spectrum/piecewise_linear.hpp>
#include <wt/math/distribution/piecewise_linear_distribution.hpp>

#include <wt/math/common.hpp>
#include <wt/scene/loader/node_readers.hpp>
#include <wt/util/format/enum.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace sensor::response;


const f_t RGB_t::f(std::uint32_t channel, const wavenumber_t& k) const noexcept {
    assert(channel<3);

    // convert from XYZ
    const auto xyz = vec3_t{
        XYZ.f(0, k),
        XYZ.f(1, k),
        XYZ.f(2, k),
    };
    const auto rgb = conversion * xyz;
    return m::max<f_t>(0,rgb[channel]);
}


scene::element::info_t RGB_t::description() const {
    using namespace scene::element;
    auto ret = info_for_scene_element(*this, "RGB", {
        { "RGB colourspace", attributes::make_enum(colourspace) },
        { "white point", attributes::make_enum(whitepoint) },
        { "XYZ", attributes::make_element(&XYZ) },
    });
    if (this->get_tonemap())
        ret.attribs.emplace("tonemap operator", attributes::make_element(this->get_tonemap()));
    return ret;
}

std::unique_ptr<response_t> RGB_t::load(std::string id, 
                                        scene::loader::loader_t* loader, 
                                        const scene::loader::node_t& node, 
                                        const wt::wt_context_t &context) {
    std::optional<std::string> colourspace_name, whitepoint_name;
    std::shared_ptr<tonemap_t> tonemap;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::read_attribute(item, "colourspace", colourspace_name) &&
            !scene::loader::read_attribute(item, "white_point", whitepoint_name) &&
            !scene::loader::load_scene_element(item, tonemap, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(RGB response loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(RGB response loader) " + std::string{ exp.what() }, item);
    }
    }

    const auto colourspace = colourspace_name ?
        format::parse_enum<colourspace::RGB_colourspace_e>(*colourspace_name) :
        default_colourspace;
    const auto whitepoint  = whitepoint_name ?
        format::parse_enum<colourspace::white_point_e>(*whitepoint_name) :
        default_white_point;

    if (!colourspace)
        throw scene_loading_exception_t("(RGB response loader) unrecognized 'colourspace'", node);
    if (!whitepoint)
        throw scene_loading_exception_t("(RGB response loader) unrecognized 'white_point'", node);

    // sRGB tonemap by default
    if (!tonemap)
        tonemap = tonemap_t::create_sRGB(id+"_sRGB_tonemap");

    return std::make_unique<RGB_t>( 
        std::move(id), context,
        std::move(tonemap),
        *colourspace, *whitepoint
    );
}
