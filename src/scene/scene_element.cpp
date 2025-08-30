/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>

#include <wt/scene/loader/node.hpp>
#include <wt/scene/loader/loader.hpp>
#include <wt/scene/element/scene_element.hpp>

#include <wt/integrator/integrator.hpp>
#include <wt/sensor/sensor.hpp>
#include <wt/sensor/response/response.hpp>
#include <wt/sensor/response/tonemap/tonemap.hpp>
#include <wt/scene/shape.hpp>
#include <wt/bsdf/bsdf.hpp>
#include <wt/emitter/emitter.hpp>
#include <wt/sampler/sampler.hpp>
#include <wt/spectrum/spectrum.hpp>
#include <wt/texture/texture.hpp>
#include <wt/texture/complex.hpp>
#include <wt/interaction/surface_profile/surface_profile.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace scene;


const std::string wt::scene::scene_element_t::empty_id = std::string{ "" };


using element_ptr_t = std::shared_ptr<scene_element_t>;

element_ptr_t scene_element_t::load(std::string id,
                                    scene::loader::loader_t* loader, 
                                    const scene::loader::node_t& node, 
                                    const wt::wt_context_t &context) {
    if (node.name() == "integrator")
        return element_ptr_t{ integrator::integrator_t::load(std::move(id), loader, node, context) };
    else if (node.name() == "sensor")
        return element_ptr_t{ sensor::sensor_t::load(std::move(id), loader, node, context) };
    else if (node.name() == "shape")
        return element_ptr_t{ shape_t::load(std::move(id), loader, node, context) };
    else if (node.name() == "bsdf")
        return element_ptr_t{ bsdf::bsdf_t::load(std::move(id), loader, node, context) };
    else if (node.name() == "emitter")
        return element_ptr_t{ emitter::emitter_t::load(std::move(id), loader, node, context) };

    else if (node.name() == "response")
        return element_ptr_t{ sensor::response::response_t::load(std::move(id), loader, node, context) };
    else if (node.name() == "tonemap")
        return element_ptr_t{ sensor::response::tonemap_t::load(std::move(id), loader, node, context) };

    else if (node.name() == "sampler")
        return element_ptr_t{ sampler::sampler_t::load(std::move(id), loader, node, context) };

    else if (node.name() == "spectrum")
        return element_ptr_t{ spectrum::spectrum_t::load(std::move(id), loader, node, context) };

    else if (node.name() == "texture")
        return element_ptr_t{ texture::texture_t::load(std::move(id), loader, node, context) };
    else if (node.name() == "texture_complex")
        return element_ptr_t{ texture::complex_t::load(std::move(id), loader, node, context) };

    else if (node.name() == "surface_profile")
        return element_ptr_t{ surface_profile::surface_profile_t::load(std::move(id), loader, node, context) };
    
    throw scene_loading_exception_t("(scene element loader) unknown type", node);
}
