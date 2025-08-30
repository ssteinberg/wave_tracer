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
#include <vector>
#include <memory>
#include <functional>

#include <wt/math/common.hpp>
#include <wt/wt_context.hpp>

#include <wt/util/unique_function.hpp>

#include <wt/texture/texture.hpp>

namespace wt::texture {

/**
 * @brief (Real-valued) texture that is an (arbitrary) function of several nested textures.
 */
class function_t final : public texture_t {
public:
    using tex_container_t = std::vector<std::shared_ptr<texture_t>>;
    using func_t = unique_function<f_t(
            const tex_container_t&,
            const texture_query_t&,
            const vec2_t&,         // uv
            const wavenumber_t     // k
        ) const noexcept>;

private:
    func_t func;
    std::string func_description;

    tex_container_t texs;

    [[nodiscard]] inline f_t eval_func(const texture_query_t& query) const {
        return func(texs,
                    query,
                    query.uv,
                    query.k);
    }

public:
    function_t(std::string id,
               tex_container_t&& texs,
               func_t&& func,
               std::string func_description)
        : texture_t(std::move(id)),
          func(std::move(func)),
          func_description(std::move(func_description)),
          texs(std::move(texs))
    {}
    function_t(std::string id,
               func_t&& func,
               std::string func_description)
        : function_t(std::move(id), {}, std::move(func), std::move(func_description))
    {}
    function_t(function_t&&) = default;
    virtual ~function_t() noexcept = default;
    
    /**
     * @brief Returns TRUE for textures that make use of the surface interaction footprint data.
     */
    [[nodiscard]] bool needs_interaction_footprint() const noexcept override {
        for (const auto& t : texs)
            if (t->needs_interaction_footprint())
                return true;
        return false;
    }
    
    /**
     * @brief Returns texture resolution represented via \f$ \frac{\text{texels}}{\vec{uv}} \f$. May return an approximation.
     *        Can be infinite, e.g., for analytic textures. Can be 1, for constant textures.
     */
    [[nodiscard]] vec2_t resolution() const noexcept override {
        auto res = vec2_t{ 1 };
        for (const auto& t : texs)
            res = m::max(res, t->resolution());
        return res;
    }

    /**
     * @brief Average spectrum of the texture. Returns ``nullptr`` when an average spectrum cannot be computed.
     */
    [[nodiscard]] std::shared_ptr<spectrum::spectrum_real_t> mean_spectrum() const noexcept override {
        // not implemented
        return nullptr;
    }

    /**
     * @brief Average value of the texture. Returns std::nullopt when an average value cannot be computed.
     */
    [[nodiscard]] std::optional<f_t> mean_value(wavenumber_t k) const noexcept override {
        return std::nullopt;
    }

    /**
     * @brief Samples the texture.
     *        Returns filtered RGBA data, without spectral upsampling. Ignores ``query.k``.
     *        Only relevant for some textures.
     */
    [[nodiscard]] vec4_t get_RGBA(const texture_query_t& query) const noexcept override {
        // not implemented
        assert(false);
        return {};
    }

    /**
     * @brief Samples the texture. Returns spectrally upsampled (to wavenumber ``query.k``) result.
     * @return Spectral luminance value and alpha (if any) pair.
     */
    [[nodiscard]] vec2_t f(const texture_query_t& query) const noexcept override {
        return vec2_t{
            eval_func(query),
            1
        };
    }
    
public:
    static std::unique_ptr<texture_t> load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
