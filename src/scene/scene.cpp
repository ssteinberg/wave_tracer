/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <vector>
#include <ranges>

#include <stdexcept>
#include <cassert>

#include <wt/scene/scene.hpp>

#include <wt/emitter/emitter.hpp>
#include <wt/emitter/infinite_emitter.hpp>

#include <wt/scene/element/attributes.hpp>

using namespace wt;
using namespace wt::scene;


scene_t::scene_t(std::string id,
                 const wt_context_t& ctx,
                 std::shared_ptr<integrator::integrator_t> integrator,
                 std::vector<std::shared_ptr<sensor::sensor_t>>&& sensors,
                 std::shared_ptr<sampler::sampler_t> sampler,
                 std::vector<std::shared_ptr<emitter::emitter_t>> emitters,
                 std::vector<std::shared_ptr<shape_t>> shapes) : 
    id(std::move(id)),
    scene_integrator(std::move(integrator)), 
    scene_emitters(std::move(emitters)), 
    scene_shapes(std::move(shapes)),
    world_aabb(aabb_t::null()),
    scene_sampler(std::move(sampler))
{
    // calculate world AABB
    for (const auto& s : scene_shapes)
        world_aabb |= s->get_mesh().get_aabb();
    assert(world_aabb.volume()>zero);

    // update emitter's indices
    for (auto idx=0ul; idx<this->scene_emitters.size(); ++idx)
        this->scene_emitters[idx]->scene_emitter_idx = idx;
    // some emitters need scene data to be set
    for (auto& em : this->scene_emitters) {
        // infinite emitters need scene AABB
        if (!em->is_infinite_emitter()) continue;
        auto* infinite_emitter = dynamic_cast<emitter::infinite_emitter_t*>(em.get());
        assert(infinite_emitter);
        if (infinite_emitter)
            infinite_emitter->set_world_aabb(world_aabb);
    }

    if (sensors.empty())
        throw std::runtime_error("(scene) no sensors provided.");
    if (sensors.size() > max_supported_sensors)
        throw std::runtime_error("(scene) sensor count exceeds limit.");

    // build emitter sampling data per each sensor
    for (auto&& s : sensors)
        this->scene_sensors.emplace(ctx, std::move(s), this);
}

scene::element::info_t scene_t::description() const {
    using namespace scene::element;

    std::vector<attribute_ptr> sensors, emitters, shapes;
    for (const auto& s : this->sensors())
        sensors.emplace_back(attributes::make_element(s.get_sensor()));
    for (const auto& e : this->emitters())
        emitters.emplace_back(attributes::make_element(e.get()));
    for (const auto& s : this->shapes())
        shapes.emplace_back(attributes::make_element(s.get()));

    return {
        .id = get_id(),
        .cls = "scene",
        .attribs = {
            { "integrator", attributes::make_element(&integrator()) },
            { "sensors",    attributes::make_array(std::move(sensors))},
            { "emitters",   attributes::make_array(std::move(emitters))},
            { "shapes",     attributes::make_array(std::move(shapes))},
        }
    };
}
