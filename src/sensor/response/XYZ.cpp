/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <stdexcept>

#include <wt/wt_context.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/sensor/response/response.hpp>
#include <wt/sensor/response/XYZ.hpp>
#include <wt/sensor/response/multichannel.hpp>

#include <wt/spectrum/piecewise_linear.hpp>
#include <wt/math/distribution/piecewise_linear_distribution.hpp>

#include <wt/spectrum/uniform.hpp>
#include <wt/spectrum/util/spectrum_from_db.hpp>

#include <wt/math/common.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace sensor::response;


// loads XYZ spectra and creates the corresponding multichannel response
inline auto create_XYZ_multichannel_response(const wt::wt_context_t &context,
                                             const std::string& id) {
    auto spectra = spectrum::response_spectrum_from_db(context, "XYZ", 3);
    assert(spectra.channels.size()==3);

    std::vector<std::shared_ptr<spectrum::spectrum_real_t>> channels;
    channels.reserve(3);
    for (auto c=0;c<3;++c) {
        std::shared_ptr<spectrum::spectrum_real_t> s;
        if (c<spectra.channels.size()) {
            auto spectrum = std::shared_ptr<spectrum::spectrum_t>{ std::move(spectra.channels[c]) };
            s = std::dynamic_pointer_cast<spectrum::spectrum_real_t>(std::move(spectrum));
            if (!s)
                throw std::runtime_error("(XYZ response loader) response_spectrum_from_db() returned a non-real spectrum");
        } else {
            s = std::make_shared<spectrum::uniform_t>(id+"_default_constant",0);
        }
        channels.emplace_back(std::move(s));
    }

    return multichannel_t{ id+"_multichannel", std::move(channels) };
}


XYZ_t::XYZ_t(std::string id,
             const wt::wt_context_t &context)
    : response_t(std::move(id), nullptr),
      response(create_XYZ_multichannel_response(context, get_id()))
{
    assert(this->response.pixel_layout().components==3);
}


scene::element::info_t XYZ_t::description() const {
    using namespace scene::element;
    auto ret = info_for_scene_element(*this, "XYZ", {
        { "channels", attributes::make_element(&response) },
    });
    if (this->get_tonemap())
        ret.attribs.emplace("tonemap operator", attributes::make_element(this->get_tonemap()));
    return ret;
}

std::unique_ptr<response_t> XYZ_t::load(std::string id,
                                        scene::loader::loader_t* loader,
                                        const scene::loader::node_t& node,
                                        const wt::wt_context_t &context) {
    return std::make_unique<XYZ_t>(
        std::move(id), context
    );
}
