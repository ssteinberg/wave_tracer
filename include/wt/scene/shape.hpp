/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <vector>
#include <memory>
#include <string>

#include <wt/wt_context.hpp>

#include <wt/scene/element/scene_element.hpp>
#include <wt/scene/position_sample.hpp>

#include <wt/mesh/mesh.hpp>

#include <wt/sampler/sampler.hpp>
#include <wt/sampler/measure.hpp>

#include <wt/interaction/intersection.hpp>

#include <wt/math/common.hpp>
#include <wt/math/transform/transform.hpp>
#include <wt/math/distribution/discrete_distribution.hpp>

namespace wt {

namespace bsdf { class bsdf_t; }
namespace emitter { class emitter_t; }

/**
 * @brief Contains a triangular mesh, a BSDF, and an optional area emitter. Provides surface sampling facilities.
 */
class shape_t final : public scene::scene_element_t {
public:
    static constexpr std::string scene_element_class() noexcept { return "shape"; }

    struct triangle_sampling_data_t {
        discrete_distribution_t<area_t> triangle_surface_area_distribution;
        area_t surface_area;
        area_density_t recp_surface_area;
    };

private:
    using bsdf_t = bsdf::bsdf_t;
    using emitter_t = emitter::emitter_t;
    using mesh_t = mesh::mesh_t;

private:
    std::shared_ptr<bsdf_t> bsdf;
    std::shared_ptr<emitter_t> emitter;
    mesh_t mesh;

    triangle_sampling_data_t sampling_data;

public:
    shape_t(std::string id,
            std::shared_ptr<bsdf_t> bsdf,
            std::shared_ptr<emitter_t> emitter, 
            mesh_t mesh);
    shape_t(shape_t&&)=default;
    shape_t(const shape_t&)=default;

    [[nodiscard]] const auto& get_bsdf() const    { return *bsdf; }
    [[nodiscard]] const auto& get_emitter() const { return emitter; }
    [[nodiscard]] const auto& get_mesh() const    { return mesh; }

    [[nodiscard]] inline auto get_surface_area() const noexcept { return sampling_data.surface_area; }

    /**
     * @brief Samples a position on the shape
     */
    [[nodiscard]] position_sample_t sample_position(sampler::sampler_t& sampler) const noexcept;

    [[nodiscard]] inline area_sampling_pd_t pdf_position(const pqvec3_t& p) const noexcept {
        return sampling_data.recp_surface_area;
    }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::shared_ptr<shape_t> load(
            std::string id, 
            scene::loader::loader_t* loader, 
            const scene::loader::node_t& node, 
            const wt::wt_context_t &context);
    static mesh_t load_mesh(const scene::loader::node_t& node, 
                            const std::string& shape_id,
                            const transform_d_t& to_world,
                            const wt::wt_context_t &context,
                            std::vector<const scene::loader::node_t*>& consumed_attributes);
};

}
