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

#include <wt/scene/scene.hpp>
#include <wt/sampler/uniform.hpp>

#include <wt/sensor/sensor/film_backed_sensor.hpp>
#include <wt/sensor/mask/mask.hpp>

#include <wt/scene/loader/node_readers.hpp>

#include <regex>

using namespace wt;
using namespace sensor::mask;


bitmap::bitmap2d_t<float> mask_t::create_mask(
        const wt::wt_context_t &context,
        const ads::ads_t& ads,
        const scene_t& scene,
        const sensor::sensor_t& sensor) const {
    const auto* fbs = dynamic_cast<const film_backed_sensor_generic_t<2>*>(&sensor);
    if (!fbs)
        throw std::runtime_error("(sensor mask) only supports 2D film-backed sensors.");
    const auto size = vec2u32_t{ fbs->resolution() };

    sampler::uniform_t sampler{};
    const std::regex regex{ this->geo_mask_id_regex };
    auto bmp = bitmap::bitmap2d_t<float>::create(size, bitmap::pixel_layout_e::L);

    std::vector<std::future<void>> fs;
    fs.resize(size.y);
    for (auto y=0ul;y<size.y;++y) {
        fs[y] = context.threadpool->enqueue([y, size, this, &ads, &scene, &sampler, &bmp, &regex, fbs]() {
            for (auto x=0ul;x<size.x;++x) {
                bmp(x,y,0) = 0;
                for (auto s=0ul;s<this->samples;++s) {
                    const auto ss = fbs->sample(sampler, { x,y,0 }, 0 / u::mm);
                    const auto intrs = ads.intersect(ss.beam.mean_ray());
                    if (intrs.empty())
                        continue;

                    const auto& t = ads.tri(*intrs.triangles().begin());
                    const auto& shape = scene.shapes()[t.shape_idx];
                    if (!std::regex_match(shape->get_id(), regex))
                        bmp(x,y,0) += 1/float(samples);
                }
            }
        });
    }
    for (auto& f : fs)
        f.get();

    return bmp;
}

scene::element::info_t mask_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "mask", {
        { "mode", attributes::make_enum(mode) }
    });
}


std::unique_ptr<mask_t> mask_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    const auto& type = node["type"];
    mask_mode_e mode;
    if (type=="by-geometry")   mode = mask_mode_e::by_geometry;
    else
        throw std::runtime_error("(sensor mask loader) Unrecognized 'type'");

    std::string geo_mask_id_regex;
    std::size_t samples;

    try {
    for (auto& item : node.children_view()) {
        if (!scene::loader::read_attribute(item, "mask_id_regex", geo_mask_id_regex) &&
            !scene::loader::read_attribute(item, "samples", samples))
            logger::cwarn()
                << loader->node_description(item)
                << "(sensor mask loader) Unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    }
    } catch(const std::format_error& exp) {
        throw std::runtime_error("(sensor mask loader) " + std::string{ exp.what() });
    }

    if (geo_mask_id_regex.empty())
        throw scene_loading_exception_t("(sensor mask loader) expected 'mask_id_regex' regex expression to be provided", node);
    
    return std::make_unique<mask_t>(
        std::move(id), mode, std::move(geo_mask_id_regex)
    );
}
