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
#include <wt/spectrum/rgb.hpp>

#include <wt/math/common.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace wt::spectrum;


std::unique_ptr<binned_piecewise_linear_distribution_t> rgb_t::rgb_to_binned_linear_distribution(
        const vec3_t& rgb) noexcept {
    constexpr auto bin_num = 1024ul;
    constexpr auto k_max = wavelen_to_wavenum(rgb_t::lambda_min);
    constexpr auto k_min = wavelen_to_wavenum(rgb_t::lambda_max);
    constexpr auto bin_len = (k_max-k_min) / f_t(bin_num);

    std::vector<f_t> vals;
    vals.reserve(bin_num+1);
    for (auto i=0ul;;++i) {
        const auto k = k_min + f_t(i)*bin_len;
        if (k>k_max) break;
        vals.emplace_back(colourspace::RGB_to_spectral::uplift(rgb, wavenum_to_wavelen(k)));
    }

    const auto k_range = range_t{
        u::to_inv_mm(k_min),
        u::to_inv_mm(k_max)
    };

    return std::make_unique<binned_piecewise_linear_distribution_t>(std::move(vals), k_range);
}


scene::element::info_t rgb_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "rgb", {
        { "RGB", attributes::make_vector(rgb) },
    });
}


inline auto read_rgb(const std::string& str, const scene::loader::node_t& node) {
    std::string l;
    std::stringstream ss{ str };
    auto rgb = vec3_t{ 0 };
    auto i=0ul;
    for (; std::getline(ss, l, ','); ++i) {
        if (i==3) { i=0; break; } 
        rgb[i] = stof_strict(l);
    }
    if (i!=3) throw scene_loading_exception_t("(rgb spectrum loader) malformed 'rgb'", node);

    return rgb;
}

std::unique_ptr<spectrum_real_t> rgb_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    std::optional<f_t> scale;
    vec3_t rgb;

    if (node.children().empty()) {
        rgb = read_rgb(node["rgb"], node);
        scale = stof_strict(node["scale"], f_t(1));
    } else {
        for (auto& item : node.children_view()) {
        try {
            if (std::string {item.name() } == "rgb")
                rgb = read_rgb(item["value"], item);
            else if (!scene::loader::read_attribute(item, "scale", scale))
                logger::cwarn()
                    << loader->node_description(item)
                    << "(rgb spectrum loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
        } catch(const std::format_error& exp) {
            throw scene_loading_exception_t("(rgb spectrum loader) " + std::string{ exp.what() }, item);
        }
        }
    }

    if (scale && *scale<0)
        throw scene_loading_exception_t("(rgb spectrum loader) 'scale' must be non-negative", node);

    return std::make_unique<rgb_t>(std::move(id), scale.value_or(f_t(1)) * rgb);
}
