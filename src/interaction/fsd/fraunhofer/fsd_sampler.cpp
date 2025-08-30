/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <memory>
#include <string>

#include <wt/interaction/fsd/fraunhofer/fsd_sampler.hpp>

#include <wt/interaction/fsd/fraunhofer/fsd.hpp>
#include <wt/interaction/fsd/fraunhofer/fsd_lut.hpp>

#include <wt/sampler/sampler.hpp>
#include <wt/math/common.hpp>

using namespace wt;
using namespace wt::fraunhofer;
using namespace wt::fraunhofer::fsd_sampler;


struct wt::fraunhofer::fsd_sampler::fsd_sample_impl_t {
    fsd_lut_t fsd1_lut;

    fsd_sample_impl_t(const wt::wt_context_t &context) : fsd1_lut(context) {}

    [[nodiscard]] vec2_t sampleP0(sampler::sampler_t& sampler) const noexcept;
    [[nodiscard]] vec2_t sample1(sampler::sampler_t& sampler,
                                 const fsd::edge_t& e) const noexcept;
    [[nodiscard]] vec2_t sampleN(sampler::sampler_t& sampler,
                                 const fsd::fsd_aperture_t& aperture) const noexcept;
};

inline vec2_t fsd_sample_impl_t::sampleP0(sampler::sampler_t& sampler) const noexcept {
    return fsd::P0_sigma * sampler.normal2d();
}

inline vec2_t fsd_sample_impl_t::sample1(sampler::sampler_t& sampler,
                                         const fsd::edge_t& e) const noexcept {
    const auto invXi = m::inverse(e.Xi());

    // sample one of the 𝛂1,𝛂2 functions of the diffracted lobes
    const f_t A = std::norm(e.a_b), 
              B = std::norm(e.iab_2);
    const auto tosample = sampler.discrete<2>({ A,B }).second;

    const vec2_t zeta =
        tosample==0 ? fsd1_lut.sample_A1(sampler.r3()) :
                      fsd1_lut.sample_A2(sampler.r3());
    return zeta * invXi;
}

inline vec2_t fsd_sample_impl_t::sampleN(sampler::sampler_t& sampler,
                                         const fsd::fsd_aperture_t& aperture) const noexcept {
    // select a random edge w.r.t. edge power Pj
    const auto s = sampler.discrete<true>(aperture.edges.size()+1,
                                          [&](std::size_t i) { return i==0 ? aperture.P0_pdf : aperture.edge_pdfs[i-1]; });
    if (s.second==0) {
        // sample P0
        return sampleP0(sampler);
    }
    else {
        // sample edge
        const auto edge_idx = s.second-1;
        return sample1(sampler, aperture.edges[edge_idx]);
    }
}


fsd_sampler_t::fsd_sampler_t(std::string id, const wt::wt_context_t &context) noexcept
    : scene_element_t(std::move(id)),
      pimpl(std::make_unique<fsd_sample_impl_t>(context))
{}
fsd_sampler_t::fsd_sampler_t(fsd_sampler_t&& o) noexcept : scene_element_t(std::move(o)), pimpl(std::move(o.pimpl)) {}
fsd_sampler_t::~fsd_sampler_t() noexcept {}

inline sample_ret_t sample_rejection(sampler::sampler_t& sampler,
                                     const fsd::fsd_aperture_t& aperture,
                                     const fsd_sample_impl_t* pimpl) noexcept {
    const auto edge_count = aperture.edges.size();

    // single edge: LUT provides virtually exact sampling
    // multiple-edges: rejection sample incoherent sum of edges' scattering functions (provides an M=edge_count upper bound on correct scattering function)
    const bool rejection_sample = edge_count>1;
    const auto M = edge_count, max_tries = M*1024ul;
    const auto recp_M = f_t(1) / M;

    for (auto tr=0ul;tr<max_tries;++tr) {
        const auto xi = pimpl->sampleN(sampler, aperture);
        const auto g  = fsd::sampling_density(aperture, xi);
        const auto f  = fsd::ASF(aperture, xi);
        assert(g>=0 && f>=0);

        const bool done = 
            rejection_sample ?
                sampler.r()*g < f*recp_M :
                true;

        if (done) {
            return {
                .xi = xi,
                .pdf = f * aperture.recp_I,
                .weight = 1,
            };
        }
    }

    return {};
}

inline sample_ret_t sample_SIR(sampler::sampler_t& sampler,
                               const fsd::fsd_aperture_t& aperture,
                               const fsd_sample_impl_t* pimpl) noexcept {
    static thread_local std::vector<vec2_t> xis;
    static thread_local std::vector<f_t> ws;
    static thread_local std::vector<f_t> fs;
    xis.clear();
    ws.clear();
    fs.clear();

    const auto edge_count = aperture.edges.size();
    const auto M = 4*edge_count;

    f_t W = 0;    
    for (auto m=0ul;m<M;++m) {
        const auto xi = pimpl->sampleN(sampler, aperture);
        const auto g  = fsd::sampling_density(aperture, xi);
        const auto f  = fsd::ASF(aperture, xi);
        assert(g>=0 && f>=0);
        const auto w  = g!=0 ? f/g : 0;

        xis.push_back(xi);
        ws.push_back(w);
        fs.push_back(f);
        W += w;
    }

    // resample
    const auto Wnorm = 1 / W;
    const auto i = sampler.discrete<true>(M, [&](std::size_t m) { return ws[m] * Wnorm; }).second;

    return {
        .xi = xis[i],
        .pdf = fs[i] * aperture.recp_I,
        .weight = 1,
    };
}

sample_ret_t fsd_sampler_t::sample(sampler::sampler_t& sampler,
                                   const fsd::fsd_aperture_t& aperture) const noexcept {
    return sample_rejection(sampler, aperture, pimpl.get());
}


scene::element::info_t fsd_sampler_t::description() const {
    return {
        .id = "",
        .type = "fsd_sampler",
    };
}
