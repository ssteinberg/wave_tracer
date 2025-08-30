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

#include <wt/ads/ads.hpp>

#include <wt/wt_context.hpp>
#include <wt/scene/element/scene_element.hpp>
#include <wt/scene/loader/node.hpp>
#include <wt/scene/scene.hpp>

#include <wt/bitmap/bitmap.hpp>

namespace wt::sensor {

class sensor_t;

namespace mask {

enum class mask_mode_e : std::uint8_t {
    by_geometry,
};

class mask_t : public scene::scene_element_t {
public:
    static constexpr std::string scene_element_class() noexcept { return "sensor_mask"; }

private:
    mask_mode_e mode;
    std::string geo_mask_id_regex;
    std::size_t samples;

public:
    mask_t(std::string id,
           mask_mode_e mode,
           std::string geo_mask_id_regex,
           std::size_t samples = 32)
        : scene_element_t(std::move(id)),
          mode(mode),
          geo_mask_id_regex(std::move(geo_mask_id_regex)),
          samples(samples)
    {}

    [[nodiscard]] bitmap::bitmap2d_t<float> create_mask(const wt::wt_context_t &context,
                                                        const ads::ads_t& ads,
                                                        const scene_t& scene,
                                                        const sensor::sensor_t& sensor) const;

public:
    static std::unique_ptr<mask_t> load(
            std::string id, 
            scene::loader::loader_t* loader, 
            const scene::loader::node_t& node, 
            const wt::wt_context_t &context);

    [[nodiscard]] scene::element::info_t description() const override;
};

}
}
