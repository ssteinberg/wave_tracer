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
#include <wt/scene/loader/loader.hpp>

#include <wt/bsdf/bsdf.hpp>

#include <wt/bsdf/composite.hpp>
#include <wt/bsdf/dielectric.hpp>
#include <wt/bsdf/diffuse.hpp>
#include <wt/bsdf/mask.hpp>
#include <wt/bsdf/normalmap.hpp>
#include <wt/bsdf/scale.hpp>
#include <wt/bsdf/surface_spm.hpp>
#include <wt/bsdf/two_sided.hpp>

using namespace wt;
using namespace bsdf;


std::unique_ptr<bsdf_t> bsdf_t::load(std::string id, 
                                     scene::loader::loader_t* loader, 
                                     const scene::loader::node_t& node, 
                                     const wt::wt_context_t &context) {
    const std::string type = node["type"];

    // convenience shorthands that can be used to define a bsdf:
    // scale
    const std::string scale = node["scale"];

    if ((scale.length()>0) + (type!="") > 1) 
        throw scene_loading_exception_t("(bsdf loader) conflicting bsdf type definition", node);

    if (type=="composite")
        return composite_t::load(std::move(id), loader, node, context);
    else if (type=="dielectric")
        return dielectric_t::load(std::move(id), loader, node, context);
    else if (type=="diffuse")
        return diffuse_t::load(std::move(id), loader, node, context);
    else if (type=="mask")
        return mask_t::load(std::move(id), loader, node, context);
    else if (type=="normalmap")
        return normalmap_t::load(std::move(id), loader, node, context);
    else if (scale.length()>0 || type=="scale")
        return scale_t::load(std::move(id), loader, node, context);
    else if (type=="surface_spm")
        return surface_spm_t::load(std::move(id), loader, node, context);
    else if (type=="twosided")
        return two_sided_t::load(std::move(id), loader, node, context);

    throw scene_loading_exception_t("(bsdf loader) unrecognized bsdf type", node);
}
