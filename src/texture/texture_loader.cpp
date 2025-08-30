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

#include <wt/texture/texture.hpp>
#include <wt/texture/bitmap.hpp>
#include <wt/texture/checkerboard.hpp>
#include <wt/texture/constant.hpp>
#include <wt/texture/function.hpp>
#include <wt/texture/mix.hpp>
#include <wt/texture/scale.hpp>
#include <wt/texture/transform.hpp>

#include <wt/texture/complex.hpp>
#include <wt/texture/complex_constant.hpp>
#include <wt/texture/complex_container.hpp>

#include <wt/math/common.hpp>

#include <wt/scene/loader/node_readers.hpp>

using namespace wt;
using namespace texture;


std::shared_ptr<texture_t> texture_t::load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context) {
    const std::string& type = node["type"];

    // convenience shorthands that can be used to define a spectrum:
    // bitmap
    const std::string& bitmap = node["bitmap"];
    // scale
    const std::string& scale = node["scale"];
    // function
    const std::string& function = node["function"];

    if (int(bitmap.length()>0) + (function.length()>0) + (scale.length()>0) + (type!="") > 1) 
        throw scene_loading_exception_t("(texture loader) conflicting texture type definition", node);
    
    if (bitmap.length()>0 || type=="bitmap")
        return bitmap_t::load(std::move(id), loader, node, context);
    else if (type=="checkerboard")
        return checkerboard_t::load(std::move(id), loader, node, context);
    else if (type=="constant")
        return constant_t::load(std::move(id), loader, node, context);
    else if (function.length()>0 || type=="function")
        return function_t::load(std::move(id), loader, node, context);
    else if (type=="mix")
        return mix_t::load(std::move(id), loader, node, context);
    else if (scale.length()>0 || type=="scale")
        return scale_t::load(std::move(id), loader, node, context);
    else if (type=="transform")
        return transform_t::load(std::move(id), loader, node, context);

    throw scene_loading_exception_t("(texture loader) unrecognized texture type", node);
}

std::shared_ptr<complex_t> complex_t::load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context) {
    const std::string& type = node["type"];

    if (type=="constant")
        return complex_constant_t::load(std::move(id), loader, node, context);
    else if (type=="container")
        return complex_container_t::load(std::move(id), loader, node, context);

    throw scene_loading_exception_t("(complex texture loader) unrecognized texture type", node);
}
