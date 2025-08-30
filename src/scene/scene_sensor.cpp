/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <cassert>

#include <wt/scene/scene.hpp>
#include <wt/emitter/emitter.hpp>

using namespace wt;
using namespace wt::scene;


const emitter::emitter_t* scene_sensor_t::emitter_sampling_data_t::sample(const scene_t& parent, sampler::sampler_t& sampler) const noexcept {
    const auto i = emitters_power_distribution.icdf(sampler.r());
    const auto* ret = parent.emitters()[i].get();
    assert(ret->scene_emitter_idx == i);

    return ret;
}


scene_sensor_t::scene_sensor_t(const wt_context_t& ctx,
                               std::shared_ptr<sensor::sensor_t> sensor,
                               const scene_t* scene)
    : sensor(std::move(sensor)),
      emitter_sampler(scene_sensor_t::emitter_sampling_data_t::build_sampling_data(ctx, this->sensor.get(), scene))
{}

emitter_wavenumber_sample_t scene_sensor_t::sample_emitter_and_spectrum(
        const scene_t& parent, 
        sampler::sampler_t& sampler) const noexcept {
    // choose an emitter
    const auto* emitter = emitter_sampler.sample(parent, sampler);
    const auto ep = pdf_emitter(parent, emitter);
    assert(ep>zero);

    // sample wavenumber
    const auto wavenumber_sample = emitter_sampler.sample_wavenumber(sampler, emitter);
    const auto k = wavenumber_sample.x / u::mm;

    const auto wpd = wavenumber_sample.measure==measure_e::discrete ? 
        wavenumber_sampling_pd_t::discrete(wavenumber_sample.pdf) :
        wavenumber_sampling_pd_t{ wavenumber_sample.pdf * u::mm };

    return {
        .emitter = emitter,
        .emitter_pdf = ep,
        .wavenumber = {
            .k = k,
            .wpd = wpd,
        },
    };
}
