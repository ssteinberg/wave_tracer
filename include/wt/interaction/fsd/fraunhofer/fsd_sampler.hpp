/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <memory>
#include <string>

#include <wt/scene/element/scene_element.hpp>
#include <wt/sampler/sampler.hpp>

#include <wt/math/common.hpp>
#include <wt/interaction/fsd/fraunhofer/fsd.hpp>

namespace wt::fraunhofer {

class scene_loader;
    
namespace fsd_sampler {

struct sample_ret_t {
    vec2_t xi;
    f_t pdf=0;
    f_t weight=0;
};

struct fsd_sample_impl_t;

/**
 * @brief FSD sampler. 
 *        The internals work in wavelength-agnostic canonical space (scaled by wavenumber k).
 */
class fsd_sampler_t : public scene::scene_element_t {
private:
    std::unique_ptr<fsd_sample_impl_t> pimpl;

public:
    fsd_sampler_t(std::string id, const wt::wt_context_t &context) noexcept;
    fsd_sampler_t(fsd_sampler_t&&) noexcept;
    ~fsd_sampler_t() noexcept;

    /**
     * @brief Samples the free-space diffraction ASF, given an aperture. 
     *        The returned weight is asf/pdf (i.e., cosine foreshortening is NOT accounted for). 
     */
    [[nodiscard]] sample_ret_t sample(sampler::sampler_t& sampler,
                                      const fsd::fsd_aperture_t& aperture) const noexcept;

    /**
     * @brief Queries the sampling density of the free-space diffraction ASF, given an aperture. 
     *        (approximation)
     */
    [[nodiscard]] f_t pdf(const fsd::fsd_aperture_t& aperture,
                          const vec2_t xi) const noexcept {
        return fsd::ASF(aperture, xi) * aperture.recp_I;
    }

    [[nodiscard]] scene::element::info_t description() const override;
};

}
}
