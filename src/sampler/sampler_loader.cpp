/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/wt_context.hpp>
#include <wt/scene/loader/loader.hpp>
#include <wt/scene/loader/node.hpp>

#include <wt/sampler/sampler.hpp>
#include <wt/sampler/sobolld.hpp>
#include <wt/sampler/uniform.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace sampler;


std::shared_ptr<sampler_t> sampler_t::load(std::string id, scene::loader::loader_t* loader, const scene::loader::node_t& node, const wt::wt_context_t &context) {
    const std::string& type = node["type"];

    if (type=="independent" || type=="uniform")
        return uniform_t::load(std::move(id), loader, node, context);
    if (type=="sobolld")
        return sobolld_t::load(std::move(id), loader, node, context);

    throw scene_loading_exception_t("(sampler loader) unrecognized sampler type", node);
}
