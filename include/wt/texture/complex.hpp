/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <string>
#include <memory>
#include <optional>

#include <wt/scene/element/scene_element.hpp>
#include <wt/texture/texture.hpp>
#include <wt/spectrum/spectrum.hpp>

#include <wt/interaction/intersection.hpp>
#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

namespace wt::texture {

/**
 * @brief Generic (complex-valued) texture.
 */
class complex_t : public scene::scene_element_t {
public:
    static constexpr std::string scene_element_class() noexcept { return "texture_complex"; }

public:
    complex_t(std::string id)
        : scene_element_t(std::move(id))
    {}
    complex_t(complex_t&&) = default;
    virtual ~complex_t() noexcept = default;
    
    /**
     * @brief Returns TRUE for textures that make use of the surface interaction footprint data.
     */
    [[nodiscard]] virtual bool needs_interaction_footprint() const noexcept = 0;
    
    /**
     * @brief Returns texture resolution represented via \f$ \frac{\text{texels}}{\vec{uv}} \f$. May return an approximation.
     *        Can be infinite, e.g., for analytic textures. Can be 1, for constant textures.
     */
    [[nodiscard]] virtual vec2_t resolution() const noexcept = 0;
    
    /**
     * @brief Returns TRUE for textures that are constant, i.e. admit a ``resolution()`` of exactly 1.
     */
    [[nodiscard]] inline bool is_constant() const noexcept { return resolution()==vec2_t{ 1,1 }; }

    /**
     * @brief Average spectrum of the texture. Returns ``nullptr`` when an average spectrum cannot be computed.
     */
    [[nodiscard]] virtual std::shared_ptr<spectrum::spectrum_t> mean_spectrum() const noexcept = 0;

    /**
     * @brief Average value of the texture. Returns std::nullopt when an average value cannot be computed.
     */
    [[nodiscard]] virtual std::optional<c_t> mean_value(wavenumber_t k) const noexcept = 0;

    /**
     * @brief Samples the texture.
     */
    [[nodiscard]] virtual c_t f(const texture_query_t& query) const noexcept = 0;
    
public:
    static std::shared_ptr<complex_t> load(
            std::string id, 
            scene::loader::loader_t* loader, 
            const scene::loader::node_t& node, 
            const wt::wt_context_t &context);
};

}
