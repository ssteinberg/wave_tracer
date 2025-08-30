/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <memory>
#include <wt/util/seeded_mt19937_64.hpp>

#include <wt/sampler/uniform.hpp>

#include <wt/scene/element/attributes.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace sampler;


thread_local seeded_mt19937_64 uniform_t::rd;

scene::element::info_t uniform_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "uniform");
}

std::unique_ptr<uniform_t> uniform_t::load(std::string id, scene::loader::loader_t* loader, const scene::loader::node_t& node, const wt::wt_context_t &context) {
    for (auto& item : node.children_view()) {
        logger::cwarn()
            << loader->node_description(item)
            << "(uniform sampler loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    }

    return std::make_unique<uniform_t>( std::move(id) );
}
