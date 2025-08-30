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

#include <wt/scene/element/scene_element.hpp>
#include <wt/scene/loader/node.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/math/common.hpp>
#include <wt/util/concepts.hpp>
#include <wt/wt_context.hpp>
#include <wt/scene/element/attributes.hpp>

#include <wt/texture/texture.hpp>

#include <wt/util/logger/logger.hpp>

namespace wt::texture {

/**
 * @brief Simple wrapper around texture_t that scales the queried texture value by a scalar quantity. Useful when textures with physical units are required.
 *        *This element should only be nested within other elements*.
 */
template <Quantity Q>
class quantity_t final : public scene::scene_element_t {
public:
    static constexpr std::string scene_element_class() noexcept { return "quantity_texture"; }

private:
    Q quantity;
    std::shared_ptr<texture_t> tex;

public:
    quantity_t(std::string id,
               std::shared_ptr<texture_t> tex,
               Q q = f_t(1) * Q::unit)
        : scene_element_t(std::move(id)),
          quantity(q),
          tex(std::move(tex))
    {}
    quantity_t(quantity_t&&) = default;
    quantity_t(const quantity_t&) = default;
    
    /**
     * @brief Returns TRUE for textures that make use of the surface interaction footprint data.
     */
    [[nodiscard]] bool needs_interaction_footprint() const noexcept {
        return tex ? tex->needs_interaction_footprint() : false;
    }
    
    /**
     * @brief Returns texture resolution represented via \f$ \frac{\text{texels}}{\vec{uv}} \f$. May return an approximation.
     *        Can be infinite, e.g., for analytic textures. Can be 1, for constant textures.
     */
    [[nodiscard]] vec2_t resolution() const noexcept {
        return tex->resolution();
    }
    
    /**
     * @brief Returns TRUE for textures that are constant, i.e. admit a ``resolution()`` of exactly 1.
     */
    [[nodiscard]] inline bool is_constant() const noexcept { return resolution()==vec2_t{ 1,1 }; }

    /**
     * @brief Average spectrum of the texture. Returns ``nullptr`` when an average spectrum cannot be computed.
     */
    [[nodiscard]] std::shared_ptr<spectrum::spectrum_real_t> mean_spectrum() const noexcept {
        // not implemented
        return nullptr;
    }

    /**
     * @brief Average value of the texture. Returns std::nullopt when an average value cannot be computed.
     */
    [[nodiscard]] std::optional<Q> mean_value(wavenumber_t k) const noexcept {
        if (tex) {
            const auto tex_mv = tex->mean_value(k);
            if (!tex_mv)
                return std::nullopt;
            return *tex_mv * quantity;
        }
        return quantity;
    }

    /**
     * @brief Samples the texture.
     *        Returns filtered RGBA data, without spectral upsampling. Ignores ``query.k``.
     *        Only relevant for some textures.
     */
    [[nodiscard]] qvec4<Q> get_RGBA(const texture_query_t& query) const noexcept {
        return tex ? quantity * tex->get_RGBA(query).x : qvec4<Q>{ quantity };
    }

    /**
     * @brief Samples the texture. Returns spectrally upsampled (to wavenumber ``query.k``) result.
     * @return Spectral luminance value and alpha (if any) pair.
     */
    [[nodiscard]] auto f(const texture_query_t& query) const noexcept {
        return tex ? quantity * tex->f(query).x : quantity;
    }
    
public:
    static std::unique_ptr<quantity_t> load(
            std::string id, 
            scene::loader::loader_t* loader, 
            const scene::loader::node_t& node, 
            const wt::wt_context_t &context) {
        std::shared_ptr<texture_t> tex;
        std::optional<Q> q;

        if (node.name()=="quantity") {
            q = stoq_strict<Q>(node["value"]);
        } else {
            for (auto& item : node.children_view()) {
            try {
                if (!scene::loader::read_attribute(item, "scale", q) &&
                    !scene::loader::load_texture_element(item, tex, loader, context))
                    logger::cwarn()
                        << loader->node_description(item)
                        << "(quantity texture loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
            } catch(const std::format_error& exp) {
                throw scene_loading_exception_t("(quantity texture loader) " + std::string{ exp.what() }, item);
            }
            }
        }
        
        if (!q)
            throw std::runtime_error("(quantity texture loader) quantity 'scale' must be provided");

        return std::make_unique<quantity_t>(std::move(id), std::move(tex), *q);
    }

    [[nodiscard]] scene::element::info_t description() const override {
        using namespace scene::element;

        auto ret = info_for_scene_element(*this, "quantity", {
            { "scale",   attributes::make_scalar(quantity) }
        });
        if (tex)
            ret.attribs.emplace("texture", attributes::make_element(tex.get()));

        return ret;
    }
};

}


namespace wt::scene::loader {

// Loads a uniquely-owned quantity texture
template <Quantity Q>
inline bool load_quantity_texture_element(
        const std::string& id, 
        const scene::loader::node_t& node, 
        const std::string& name, 
        std::unique_ptr<texture::quantity_t<Q>>& out,
        scene::loader::loader_t* loader,
        const wt::wt_context_t &context) {
    using SceneElement = texture::quantity_t<Q>;
    const auto cls = SceneElement::scene_element_class();
    const auto& nodename = node.name();
    if ((nodename == cls || nodename=="quantity") && node["name"]==name) {
        if (auto p = SceneElement::load(id, loader, node, context); p) {
            if (out)
                throw std::format_error("Node of class '" + cls + "' already specified");
            out = std::move(p);
            return true;
        }
        else
            throw std::format_error("Failed loading node of class '" + cls + "'");
    }

    return false;
}

}
