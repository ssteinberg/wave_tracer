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
#include <wt/scene/loader/node_readers.hpp>
#include <wt/scene/element/attributes.hpp>
#include <wt/spectrum/gaussian.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::spectrum;


scene::element::info_t gaussian_t::description() const {
    using namespace scene::element;

    auto stddev = wavenum_to_wavelen((dist.mean()+dist.std_dev()) / u::mm);
    auto mean   = wavenum_to_wavelen(dist.mean() / u::mm);
    stddev = m::abs(stddev-mean);

    return info_for_scene_element(*this, "gaussian", {
        { "power (mean)", attributes::make_scalar(val0) },
        { "mean", attributes::make_scalar(mean) },
        { "standard deviation", attributes::make_scalar(stddev) },
        { "FWHM", attributes::make_scalar(stddev * f_t(2*m::sqrt(2*std::numbers::ln2))) },
    });
}


std::unique_ptr<spectrum_real_t> gaussian_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    f_t val;
    wavelength_t mean, stddev;

    val  = stof_strict(node["value"], 1);
    mean = parse_wavelength(node["wavelength"]);
    stddev = parse_wavelength(node["stddev"]);
    for (auto& item : node.children_view()) {
        logger::cwarn()
            << loader->node_description(item)
            << "(gaussian spectrum loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    }

    if (val<zero)
        throw scene_loading_exception_t("(gaussian spectrum loader) a non-negative value must be provided", node);
    if (stddev<zero)
        throw scene_loading_exception_t("(gaussian spectrum loader) a non-negative standard deviation 'stddev' must be provided", node);

    // convert mean and stddev to wavenumbers
    // we approximate the Gaussian distribution in k-space
    auto stddev_k = u::to_inv_mm(wavelen_to_wavenum(mean+stddev));
    const auto mean_k = u::to_inv_mm(wavelen_to_wavenum(mean));
    stddev_k = mean_k - stddev_k;
    assert(stddev_k>0);
    if (stddev_k<0) stddev_k = 0;

    // confine range to up to 10 stddevs from mean
    const auto range = range_t<>{ 0,m::inf } & range_t<>{ mean_k - 10*stddev_k, mean_k + 10*stddev_k };
    const auto g = truncated_gaussian1d_t{ stddev_k, mean_k, range };

    return std::make_unique<gaussian_t>( 
        std::move(id), 
        g, val, 
        range_t<wavenumber_t>{ range.min / u::mm, range.max / u::mm }
    );
}
