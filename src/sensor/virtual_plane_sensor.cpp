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

#include <wt/sensor/sensor/virtual_plane_sensor.hpp>
#include <wt/sensor/film/film.hpp>

#include <wt/math/transform/transform_loader.hpp>
#include <wt/sampler/measure.hpp>

#include <wt/scene/loader/node_readers.hpp>

#include <wt/math/intersect/ray.hpp>

using namespace wt;
using namespace wt::sensor;


virtual_plane_sensor_t::virtual_plane_sensor_t(const wt_context_t &ctx,
                                             std::string id,
                                             const transform_t& transform,
                                             pqvec2_t sensor_extent,
                                             film_t film,
                                             std::uint32_t samples_per_element,
                                             bool ray_trace,
                                             std::optional<f_t> requested_tan_alpha) noexcept 
    : film_backed_sensor_t(ctx,
                           std::move(id),
                           std::move(film),
                           samples_per_element,
                           ray_trace),
      sensor_frame(transform(frame_t::canonical())),
      sensor_extent(
          vec2_t{ 
              m::length(transform(vec3_t{ 1,0,0 }, transform_vector)),
              m::length(transform(vec3_t{ 0,1,0 }, transform_vector))
          } * sensor_extent),
      recp_area(f_t(1) / area()),
      requested_tan_alpha(requested_tan_alpha)
{
    // the above assumes that the transform does not skew!

    const auto sensor_centre = transform(pqvec3_t{}, transform_point);
    this->sensor_origin = 
        sensor_centre - 
        sensor_extent.x/2 * sensor_frame.t - 
        sensor_extent.y/2 * sensor_frame.b;

    this->sensor_element_extent = sensor_extent / vec2_t{ resolution() };
    this->recp_sensor_element_extent = f_t(1) / sensor_element_extent;
}

std::optional<sensor_direct_connection_t> virtual_plane_sensor_t::Si(
        const spectral_radiant_flux_beam_t& beam,
        const pqrange_t<>& range) const noexcept {
    const auto n  = frame().n;
    const auto dn = m::dot(-beam.dir(),n);

    // check side
    if (dn<=0)
        return {};

    // find an intersection
    // TODO: use full beam-plane intersection
    // TODO: beam intersection with multiple elements

    const auto& a = sensor_origin;
    const auto& b = sensor_origin + sensor_extent.x*frame().t;
    const auto& c = sensor_origin + sensor_extent.y*frame().b;
    const auto& d = sensor_origin + sensor_extent.x*frame().t + sensor_extent.y*frame().b;

    const auto& ray = beam.get_envelope().ray();
    const auto intr1 = intersect::intersect_ray_tri(ray, a,b,c, range);
    const auto intr2 = intersect::intersect_ray_tri(ray, c,b,d, range);
    if (!intr1 && !intr2)
        return {};

    // deduce position and element
    const auto& p = intr1 ? ray.propagate(intr1->dist) : ray.propagate(intr2->dist);
    const auto element = element_for_position(p);
    const auto P = ray_t{ p, -beam.dir() };

    const auto surface = intersection_surface_t{ n, p };

    return sensor_direct_connection_t{
        .beam = Se(P, beam.k()) / dn,
        .element = element,
        .surface = surface,
    };
}

sensor_sample_t virtual_plane_sensor_t::sample(sampler::sampler_t& sampler,
                                              const vec3u32_t& sensor_element,
                                              const wavenumber_t k) const noexcept {
    const auto element_offset = vec3_t{ sampler.r2() - vec2_t{ .5,.5 }, 0 };
    const auto p = position_for_element(sensor::sensor_element_sample_t{ .element=sensor_element, .offset=element_offset });
    const auto recp_ppd = area();

    const auto& wo = sampler.cosine_hemisphere();
    const auto dpd = solid_angle_density_t{ sampler.cosine_hemisphere_pdf(wo.z) / u::ang::sr };

    const auto& frame = this->frame();
    const auto P = ray_t{ p, frame.to_world(wo) };
    const auto beam = Se(P, k) * recp_ppd * (dpd>zero ? 1/dpd : solid_angle_t::zero());

    const auto surface = intersection_surface_t{ frame.n, p };

    return sensor_sample_t{
        .sensor = this,

        .beam = beam,
        .ppd = area_density_t{ 1/recp_ppd },
        .dpd = dpd,

        .element = sensor_element_sample_t{
            .element = sensor_element,
            .offset  = element_offset,
        },

        .surface = surface,
    };
}

sensor_direct_sample_t virtual_plane_sensor_t::sample_direct(sampler::sampler_t& sampler,
                                                            const pqvec3_t& wp,
                                                            const wavenumber_t k) const noexcept {
    // sample point on sensor
    const auto& frame = this->frame();
    const auto splocal = sampler.r2() * sensor_extent;
    const auto sp = sensor_origin + splocal.x*frame.t + splocal.y*frame.b;

    // point's element and element offset
    const auto element_fp = vec2_t{ splocal / element_extent() };
    const auto film_element = vec2u32_t{ element_fp };
    const auto element_offset = element_fp - vec2_t{ film_element } - vec2_t{ .5,.5 };

    // direction
    const auto wdl = wp - sp;
    const auto dist2 = m::length2(wdl);
    const auto wd = dir3_t{ wdl / m::sqrt(dist2) };
    const auto wd_local = frame.to_local(wd);
    const auto recp_dn = (wd_local.z>0 ? 1/wd_local.z : 0) / u::ang::sr;

    const auto dpd = solid_angle_density_t{ recp_area * dist2 * recp_dn };
    const auto recp_dpd = dpd>zero ? 1/dpd : solid_angle_t::zero();

    const auto P = ray_t{ sp, wd };
    const auto beam = Se(P, k) * recp_dpd * recp_dn;

    const auto surface = intersection_surface_t{ frame.n, sp };

    return sensor_direct_sample_t{
        .sensor = this,

        .beam = beam,
        .dpd = dpd,

        .element = sensor_element_sample_t{
            .element = { film_element, 0 },
            .offset  = { element_offset, 0 },
        },

        .surface = surface,
    };
}

area_sampling_pd_t virtual_plane_sensor_t::pdf_position(const pqvec3_t& p) const noexcept {
    return area_sampling_pd_t{ recp_area };
}

solid_angle_sampling_pd_t virtual_plane_sensor_t::pdf_direction(const pqvec3_t& p, const dir3_t& dir) const noexcept {
    const auto d = frame().to_local(dir);
    const auto cosine = d.z;
    return solid_angle_density_t{ sampler::sampler_t::cosine_hemisphere_pdf(m::max<f_t>(cosine,0)) / u::ang::sr };
}


scene::element::info_t virtual_plane_sensor_t::description() const {
    using namespace scene::element;

    auto film_info = film().description();
    auto film_desc = std::move(film_info.attribs);
    film_desc.emplace("cls", attributes::make_string(film_info.cls));

    const auto& frame = this->frame();

    return info_for_scene_element(*this, "virtual_plane", {
        { "centre",  attributes::make_vector(centre()) },
        { "up",      attributes::make_vector(frame.n) },
        { "x",       attributes::make_vector(frame.t) },
        { "y",       attributes::make_vector(frame.b) },
        { "extent",  attributes::make_vector(extent()) },
        { "film",    attributes::make_map(std::move(film_desc)) },
    });
}



std::shared_ptr<virtual_plane_sensor_t> virtual_plane_sensor_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    if (node["type"]!="virtual_plane")
        throw scene_loading_exception_t("(virtual_plane sensor loader) unsupported sensor type", node);

    transform_t to_world = {};
    pqvec2_t extent = { 1*u::m,1*u::m };

    std::uint32_t samples_per_element;
    std::optional<film_t> film;

    std::optional<angle_t> requested_alpha;

    bool ray_trace = false;

    for (auto& item : node.children_view()) {
    try {
        if (item.name() == "film") {
            film.emplace(film_t::load(loader, item, beam_source_spatial_stddev, context));
        }
        else if (!scene::loader::read_attribute(item, "samples", samples_per_element) &&
                 !scene::loader::read_attribute(item, "extent", extent) &&
                 !scene::loader::load_transform(item, "to_world", to_world, loader) &&
                 !scene::loader::read_attribute(item, "ray_trace_only", ray_trace) &&
                 !scene::loader::read_attribute(item, "alpha", requested_alpha))
            logger::cwarn()
                << loader->node_description(item)
                << "(virtual_plane sensor loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(virtual_plane sensor loader) " + std::string{ exp.what() }, item);
    }
    }

    if (!film)
        throw scene_loading_exception_t("(virtual_plane sensor loader) film must be provided", node);
    if (m::any(m::islteqzero(extent)))
        throw scene_loading_exception_t("(virtual_plane sensor loader) extent must be positive", node);

    return std::make_shared<virtual_plane_sensor_t>( 
        context,
        std::move(id),
        to_world,
        extent,
        std::move(film).value(),
        samples_per_element,
        ray_trace || context.renderer_force_ray_tracing,
        requested_alpha ? m::tan(*requested_alpha) : std::optional<f_t>{}
    );
}
