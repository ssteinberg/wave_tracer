/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <memory>

#include <wt/scene/loader/node.hpp>
#include <wt/scene/loader/node_readers.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/spectrum/spectrum.hpp>
#include <wt/spectrum/complex_container.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::spectrum;


scene::element::info_t complex_container_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "complex_container", {
        { "real", attributes::make_element(real_spectrum.get()) },
        { "imag", imag_spectrum ? (attribute_ptr)attributes::make_element(imag_spectrum.get()) : attributes::make_scalar(0) },
    });
}


std::unique_ptr<spectrum_t> complex_container_t::load(std::string id, 
                                             scene::loader::loader_t* loader, 
                                             const scene::loader::node_t& node, 
                                             const wt::wt_context_t &context) {
    std::shared_ptr<spectrum::spectrum_t> real, imag;

    for (auto& item : node.children_view()) {
    try {
        if (!scene::loader::load_scene_element(item, "real", real, loader, context) &&
            !scene::loader::load_scene_element(item, "imag", imag, loader, context))
            logger::cwarn()
                << loader->node_description(item)
                << "(complex container spectrum loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(complex container spectrum loader) " + std::string{ exp.what() }, item);
    }
    }

    if (!real || !imag)
        throw scene_loading_exception_t("(complex container spectrum loader) 'real' and 'imag' spectra must be provided", node);
    auto rreal = std::dynamic_pointer_cast<spectrum_real_t>(std::move(real));
    auto rimag = std::dynamic_pointer_cast<spectrum_real_t>(std::move(imag));
    if (!rreal || !rimag)
        throw scene_loading_exception_t("(complex container spectrum loader) 'real' and 'imag' must be real spectra", node);

    return std::make_unique<complex_container_t>(std::move(id), std::move(rreal), std::move(rimag));
}
