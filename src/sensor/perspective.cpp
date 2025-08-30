/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <optional>
#include <utility>

#include <wt/scene/loader/node.hpp>
#include <wt/scene/element/attributes.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/sensor/sensor/perspective.hpp>
#include <wt/sensor/film/film.hpp>

#include <wt/math/transform/transform_loader.hpp>
#include <wt/sampler/measure.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::sensor;



template <bool polarimetric>
scene::element::info_t perspective_t<polarimetric>::description() const {
    using namespace scene::element;

    auto film_info = this->film().description();
    auto film_desc = std::move(film_info.attribs);
    film_desc.emplace("cls", attributes::make_string(film_info.cls));

    const auto eye = position();
    const auto dir = view_dir();
    const auto up  = up_dir();

    return info_for_scene_element(*this, "perspective", {
        { "eye",      attributes::make_vector(eye) },
        { "view direction", attributes::make_vector(dir) },
        { "up direction",   attributes::make_vector(up) },
        { "FOV",      attributes::make_scalar(fov()) },
        { "film",     attributes::make_map(std::move(film_desc)) },
    });
}
template scene::element::info_t perspective_t<false>::description() const;
template scene::element::info_t perspective_t<true>::description() const;


template <bool polarimetric>
std::shared_ptr<perspective_t<polarimetric>> perspective_t<polarimetric>::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    if (node["type"]!="perspective")
        throw scene_loading_exception_t("(perspective sensor loader) unsupported sensor type", node);

    std::optional<angle_t> fov;
    std::string fov_axis = "y";
    std::optional<length_t> focal_length;

    transform_t to_world = {};

    std::uint32_t samples_per_element;
    std::optional<film_t> film;
    std::shared_ptr<mask::mask_t> sensor_mask;

    bool ray_trace = false;
    f_t phase_space_extent_scale = 1;

    std::optional<angle_t> requested_alpha;

    for (auto& item : node.children_view()) {
    try {
        if (item.name() == "film") {
            film.emplace(film_t::load(loader, item, beam_source_spatial_stddev, context));
        }
        else if (scene::loader::read_attribute(item, "fov", fov)) {
            if (focal_length)
                throw scene_loading_exception_t("(perspective sensor loader) either 'fov' or 'focal_length' can be specified", node);
        }
        else if (scene::loader::read_attribute(item, "focal_length", focal_length)) {
            if (fov)
                throw scene_loading_exception_t("(perspective sensor loader) either 'fov' or 'focal_length' can be specified", node);
        }
        else if (!scene::loader::read_attribute(item, "samples", samples_per_element) &&
                 !scene::loader::load_transform(item, "to_world", to_world, loader) &&
                 !scene::loader::read_attribute(item, "alpha", requested_alpha) &&
                 !scene::loader::read_attribute(item, "fov_axis", fov_axis) &&
                 !scene::loader::read_attribute(item, "ray_trace_only", ray_trace) &&
                 !scene::loader::read_attribute(item, "phase_space_extent_scale", phase_space_extent_scale) &&
                 !scene::loader::load_scene_element(item, sensor_mask, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(perspective sensor loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(perspective sensor loader) " + std::string{ exp.what() }, item);
    }
    }

    if (!fov && !focal_length)
        throw scene_loading_exception_t("(perspective sensor loader) either 'fov' or 'focal_length' must be specified", node);
    if (fov && fov.value()<=0*u::ang::rad)
        throw scene_loading_exception_t("(perspective sensor loader) 'fov' must be positive", node);
    if (focal_length && focal_length.value()<=0*u::mm)
        throw scene_loading_exception_t("(perspective sensor loader) 'focal_length' must be positive", node);
    if (fov_axis != "x" && fov_axis != "y")
        throw scene_loading_exception_t("(perspective sensor loader) unsupported 'fov_axis'", node);

    if (requested_alpha && requested_alpha<=zero)
        throw scene_loading_exception_t("(perspective sensor loader) 'alpha' must be a positive real", node);

    if (!film)
        throw scene_loading_exception_t("(perspective sensor loader) film must be provided", node);

    const auto aspect = film.value().aspect_ratio();
    auto fov_value = fov ? fov.value() : 0*u::ang::rad;
    if (focal_length) {
        // convert focal length to vertical FOV
        constexpr auto image_w = f_t(36)*u::mm;
        constexpr auto image_h = f_t(24)*u::mm;
        static const length_t image_size = m::sqrt(m::sqr(image_w)+m::sqr(image_h));
        const auto dfov = (f_t)(2*image_size / (2*focal_length.value()));
        const auto yfov = dfov / m::sqrt(1 + m::sqr(aspect));
        fov_value = 2*m::atan(yfov/2);
    }

    else if (fov_axis == "x") {
        // get vertical FOV from teh given horizontal FOV
        fov_value = 2*m::atan(m::tan(fov_value/2) / aspect);
    }

    return std::make_shared<perspective_t>( 
        context,
        std::move(id),
        to_world,
        fov_value,
        std::move(film).value(),
        samples_per_element,
        ray_trace || context.renderer_force_ray_tracing,
        std::move(sensor_mask),
        requested_alpha ? m::tan(*requested_alpha) : std::optional<f_t>{},
        phase_space_extent_scale
    );
}
template std::shared_ptr<perspective_t<false>> perspective_t<false>::load(
        std::string, scene::loader::loader_t*, const scene::loader::node_t&, const wt::wt_context_t&);
template std::shared_ptr<perspective_t<true>> perspective_t<true>::load(
        std::string, scene::loader::loader_t*, const scene::loader::node_t&, const wt::wt_context_t&);
