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

#include <wt/spectrum/spectrum.hpp>
#include <wt/spectrum/complex_container.hpp>
#include <wt/spectrum/complex_analytic.hpp>
#include <wt/spectrum/complex_uniform.hpp>

#include <wt/spectrum/binned.hpp>
#include <wt/spectrum/blackbody.hpp>
#include <wt/spectrum/composite.hpp>
#include <wt/spectrum/uniform.hpp>
#include <wt/spectrum/discrete.hpp>
#include <wt/spectrum/function.hpp>
#include <wt/spectrum/gaussian.hpp>
#include <wt/spectrum/piecewise_linear.hpp>
#include <wt/spectrum/rgb.hpp>

#include <wt/spectrum/util/spectrum_from_db.hpp>
#include <wt/spectrum/util/spectrum_from_ITU.hpp>

#include <wt/math/common.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace spectrum;


std::unique_ptr<spectrum_t> spectrum_t::load(std::string id, 
                                             scene::loader::loader_t* loader, 
                                             const scene::loader::node_t& node, 
                                             const wt::wt_context_t &context) {
    const std::string& type = node["type"];

    // convenience shorthands that can be used to define a spectrum:
    // blackbody
    const std::string& blackbody = node["blackbody"];
    // constant spectrum
    const std::string& constant = node["constant"];
    // function spectrum
    const std::string& function = node["function"];
    // constant spectrum
    const std::string& rgb = node["rgb"];
    // load spectrum from db (emitter)
    const std::string& emitter = node["emitter"];
    // load spectrum from db (ior)
    const std::string& material = node["material"];
    // analytic spectrum for ITU material
    const std::string& ITU = node["ITU"];

    if (int(constant.length()>0) + (function.length()>0) + (blackbody.length()>0) + (rgb.length()>0) + (emitter.length()>0) + (material.length()>0) + (ITU.length()>0) + (type!="") > 1) 
        throw scene_loading_exception_t("(spectrum loader) conflicting spectrum type definition", node);

    if (blackbody.length()>0 && type=="")
        return blackbody_t::load(std::move(id), loader, node, context);

    if (constant.length()>0 && type=="") {
        const auto val = parse_complex_strict(constant);
        if (std::real(val)==val)
            return uniform_t::load(std::move(id), loader, node, context);
        return complex_uniform_t::load(std::move(id), loader, node, context);
    }

    if (rgb.length()>0 && type=="")
        return rgb_t::load(std::move(id), loader, node, context);

    if (ITU.length()>0 && type=="")
        return spectrum_from_ITU_material(std::move(id), ITU);

    if (emitter.length()>0 && type=="") {
        f_t scale = 1;

        for (auto& item : node.children_view()) {
            if (!scene::loader::read_attribute(item, "scale", scale))
                logger::cwarn()
                    << loader->node_description(item)
                    << "(spectrum loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
        }

        if (scale<0)
            throw scene_loading_exception_t("(spectrum loader) 'scale' must be non-negative", node);

        return emission_spectrum_from_db(context, emitter, scale);
    }

    if (material.length()>0) {
        // read IOR form database
        auto spectra = spectrum_from_material(context, material);
        assert(spectra.channels.size()>0);

        auto real = std::move(spectra.channels[0]);
        if (spectra.channels.size()==1)
            return real;
        auto imag = std::shared_ptr<spectrum_t>{ std::move(spectra.channels[1]) };

        auto rreal = std::dynamic_pointer_cast<spectrum_real_t>(std::shared_ptr<spectrum_t>(std::move(real)));
        auto rimag = std::dynamic_pointer_cast<spectrum_real_t>(std::move(imag));
        if (!rreal || !rimag)
            throw scene_loading_exception_t("(spectrum loader) 'real' and 'imag' must be real spectra", node);
        return std::make_unique<complex_container_t>(std::move(id), std::move(rreal), std::move(rimag));
    }

    if (function.length()>0 || type=="function")
        return function_t::load(std::move(id), loader, node, context);

    // explicitly defined spectrum
    if (type=="binned")
        return binned_t::load(std::move(id), loader, node, context);
    if (type=="composite")
        return composite_t::load(std::move(id), loader, node, context);
    if (type=="discrete")
        return discrete_t::load(std::move(id), loader, node, context);
    if (type=="gaussian")
        return gaussian_t::load(std::move(id), loader, node, context);
    if (type=="piecewise_linear")
        return piecewise_linear_t::load(std::move(id), loader, node, context);

    throw scene_loading_exception_t("(spectrum loader) unrecognized spectrum type", node);
}
