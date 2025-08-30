/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <format>

#include <wt/wt_context.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/sensor/response/response.hpp>
#include <wt/sensor/response/multichannel.hpp>

#include <wt/spectrum/binned.hpp>
#include <wt/spectrum/piecewise_linear.hpp>

#include <wt/math/common.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace sensor::response;


template <typename S, typename D>
inline auto ctor_spectrum_sum(std::string&& id,
                              const std::vector<std::shared_ptr<spectrum::spectrum_real_t>>& channels) {
    std::optional<D> sum;
    for (const auto& c : channels) {
        const auto s = dynamic_cast<const S*>(c.get());
        if (s) {
            const auto dist = dynamic_cast<const D*>(s->distribution());
            if (!dist)
                throw std::runtime_error("(multichannel response) channel spectra mismatch");

            if (!sum) sum = *dist;
            else      sum = sum.value() + *dist;
        }
    }
    return std::make_unique<S>(std::move(id), std::move(sum.value()));
}

multichannel_t::multichannel_t(std::string id, 
                               std::vector<std::shared_ptr<spectrum::spectrum_real_t>> chnls)
    : response_t(std::move(id), nullptr),
      channels(std::move(chnls))
{
    if (channels.empty())
        return;

    const bool chnl_has_pwl  = !!dynamic_cast<const spectrum::piecewise_linear_t*>(channels.front().get());
    const bool chnl_has_bpwl = !!dynamic_cast<const spectrum::binned_t*>(channels.front().get());

    auto sspec_id = get_id() + "_sensitivity";

    // need to compute spectrum sum
    // only support piecewise-linear spectra for now
    if (chnl_has_pwl)
        this->sensitivity_spectrum =
            ctor_spectrum_sum<spectrum::piecewise_linear_t, piecewise_linear_distribution_t>(std::move(sspec_id), channels);
    else if (chnl_has_bpwl)
        this->sensitivity_spectrum =
            ctor_spectrum_sum<spectrum::binned_t, binned_piecewise_linear_distribution_t>(std::move(sspec_id), channels);
    else
        throw std::runtime_error("(multichannel response) only piecewise-linear and binned piecewise-linear spectra are supported");
}


scene::element::info_t multichannel_t::description() const {
    using namespace scene::element;

    std::vector<attribute_ptr> channels;
    for (std::uint32_t c=0;c<pixel_layout().components;++c)
        channels.emplace_back(attributes::make_element(this->channels[c].get()));

    auto ret = info_for_scene_element(*this, "multichannel", {
        { "channels", attributes::make_array(std::move(channels)) },
    });
    if (this->get_tonemap())
        ret.attribs.emplace("tonemap operator", attributes::make_element(this->get_tonemap()));
    return ret;
}

std::unique_ptr<response_t> multichannel_t::load(std::string id, 
                                                 scene::loader::loader_t* loader, 
                                                 const scene::loader::node_t& node, 
                                                 const wt::wt_context_t &context) {
    std::vector<std::shared_ptr<spectrum::spectrum_real_t>> channels;

    for (auto& item : node.children_view()) {
    try {
        std::shared_ptr<spectrum::spectrum_real_t> channel;
        if (scene::loader::load_scene_element(item, channel, loader, context))
            channels.emplace_back(std::move(channel));
        else
            logger::cwarn()
                << loader->node_description(item)
                << "(multichannel response loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(multichannel response loader) " + std::string{ exp.what() }, item);
    }
    }

    if (channels.empty())
        throw scene_loading_exception_t("(multichannel response loader) channel spectra must be provided as child nodes", node);

    return std::make_unique<multichannel_t>( 
        std::move(id), 
        std::move(channels)
    );
}
