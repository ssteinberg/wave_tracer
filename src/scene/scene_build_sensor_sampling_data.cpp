/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <vector>
#include <future>

#include <wt/scene/scene.hpp>
#include <wt/emitter/emitter.hpp>
#include <wt/emitter/infinite_emitter.hpp>

#include <wt/util/thread_pool/tpool.hpp>
#include <wt/util/logger/logger.hpp>

#include <wt/spectrum/spectrum.hpp>
#include <wt/spectrum/uniform.hpp>
#include <wt/math/distribution/distribution1d.hpp>
#include <wt/math/distribution/product_distribution.hpp>

using namespace wt;
using namespace wt::scene;


// convert sensor-emitter product spectra to binned spectra.
// binned spectra are faster to lookup, but might be less precise or memory intensive.
static constexpr bool scene_use_binned_emitter_power_spectra = true;
static constexpr auto max_binned_spectrum_bins = 10000;


struct emitter_sensor_spectra_result_t {
    std::unique_ptr<distribution1d_t> dist;
    radiant_flux_t power;
};

scene_sensor_t::emitter_sampling_data_t scene_sensor_t::emitter_sampling_data_t::build_sampling_data(
        const wt_context_t& ctx,
        const sensor::sensor_t* sensor, 
        const scene_t* scene) {
    const auto& emitters = scene->emitters();

    if (!emitters.size())
        throw std::runtime_error("(scene) no emitters defined");

    const auto& sensor_spectrum = sensor->sensitivity_spectrum();
    const auto* sdist = sensor_spectrum.distribution();
    auto sensitivity_range = sensor_spectrum.wavenumber_range();
    if (sensitivity_range.length()==zero) {
        // slightly enlarge discrete sensitivity range to avoid NaNs, only used for integrating emission spectra
        const auto eps = sensitivity_range.centre()*f_t(1e-6);
        sensitivity_range.min -= eps;
        sensitivity_range.max += eps;
    }
    const auto all_wavenumbers = range_t<wavenumber_t>{ wavenumber_t::zero(),limits<wavenumber_t>::infinity() };

    if (!sensor_spectrum.power())
        throw std::runtime_error("(scene) sensor <" + sensor->get_id() + "> spectrum has 0 power");

    // integrate the spectrum of each emitter over the sensor's spectrum
    // and build a discrete distribution of powers contained in these integrated spectra
    // do so in parallel

    std::vector<std::future<emitter_sensor_spectra_result_t>> futures;
    futures.reserve(emitters.size());
    for (const auto& e : emitters)
    futures.emplace_back(ctx.threadpool->enqueue([&]() -> emitter_sensor_spectra_result_t {
        const auto* edist = e->emission_spectrum().distribution();

        if (!sdist)
            throw std::runtime_error("(scene) sensor <" + sensor->get_id() + "> spectrum has nullptr distribution");
        if (!edist)
            throw std::runtime_error("(scene) emitter <" + e->get_id() + "> spectrum has nullptr distribution");

        // compute emitter powers
        const auto emitter_power = e->power(all_wavenumbers);
        const auto emitter_power_over_sensitivity_range = e->power(sensitivity_range);

        if (!m::isfinite(emitter_power) || emitter_power==zero) {
            // emitters with infinite power (e.g., uniform spectrum) or 0 power
            wt::logger::cerr()
                << "(scene) emitter <" + e->get_id() + ">: 0 or ∞ emission power" 
                << '\n';

            return { nullptr, radiant_flux_t::zero() };
        }

        // calculate the overlap between the distributions of the sensitivity spectrum and emission spectrum, 
        // and compute their product distribution
        if (const auto* uniform_sspec = dynamic_cast<const spectrum::uniform_t*>(&sensor_spectrum); uniform_sspec) {
            // special handle for uniform sensitivity distributions
            const auto p = uniform_sspec->average_power() * emitter_power;

            return { edist->clone(), p };
        } else {
            // take the product spectrum
            auto ret = product_distribution(sdist, edist);

            const auto p = ret.R0 * emitter_power_over_sensitivity_range;
            assert(p>=zero && m::isfinite(p));

            std::unique_ptr<distribution1d_t> dist;
            const auto* pwld = dynamic_cast<const piecewise_linear_distribution_t*>(ret.dist.get());
            if (scene_use_binned_emitter_power_spectra && !!pwld) {
                // convert to binned spectrum
                dist = std::make_unique<binned_piecewise_linear_distribution_t>(
                        *pwld,
                        range_t<>{ u::to_inv_mm(sensitivity_range.min), u::to_inv_mm(sensitivity_range.max) },
                        u::to_inv_mm(sensitivity_range.length()) / f_t(max_binned_spectrum_bins));
            } else {
                dist = std::move(ret.dist);
            }

            return { std::move(dist), p };
        }
    }));

    std::vector<std::unique_ptr<distribution1d_t>> emitter_sensor_spectra;
    std::vector<radiant_flux_t> emitter_sensor_spectra_powers;
    emitter_sensor_spectra.reserve(emitters.size());
    emitter_sensor_spectra_powers.reserve(emitters.size());
    for (auto& f : futures) {
        auto&& ret = std::move(f).get();
        emitter_sensor_spectra.emplace_back(std::move(ret.dist));
        emitter_sensor_spectra_powers.emplace_back(ret.power);
    }

    // total power in ALL spectra
    const auto total_spectra_power = std::accumulate(
            emitter_sensor_spectra_powers.cbegin(), 
            emitter_sensor_spectra_powers.cend(), 
            radiant_flux_t::zero());
    if (total_spectra_power == radiant_flux_t::zero()) {
        wt::logger::cerr()
            << "(scene) sensor <" + sensor->get_id() + ">: no overlap between emitters' emissions spectra and sensor sensitivity spectrum" 
            << '\n';
    }

    const auto recp_total_power = total_spectra_power>zero ? 1/total_spectra_power : 0/u::W;
    return scene_sensor_t::emitter_sampling_data_t{
        .emitter_sensor_spectra = std::move(emitter_sensor_spectra),
        .emitters_power_distribution = discrete_distribution_t<radiant_flux_t>(
            std::move(emitter_sensor_spectra_powers),
            [recp_total_power](const auto p) { return f_t(p * recp_total_power); }
        ),
    };
}
