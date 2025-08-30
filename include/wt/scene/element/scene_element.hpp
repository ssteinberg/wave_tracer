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

#include <wt/wt_context.hpp>

#include <wt/scene/element/info.hpp>

namespace wt::scene::loader {
class loader_t;
class node_t;
}


namespace wt::scene {

/**
 * @brief Generic scene element.
 */
class scene_element_t {
    friend class wt::scene::loader::loader_t;

    static const std::string empty_id;

protected:
    std::unique_ptr<std::string> id;

public:
    scene_element_t(std::string id) noexcept 
        : id(id.empty() ? nullptr : std::make_unique<std::string>(std::move(id))) 
    {}
    scene_element_t(scene_element_t&&) noexcept = default;
    scene_element_t(const scene_element_t& o) noexcept
        : id(o.get_id().empty() ? nullptr : std::make_unique<std::string>(o.get_id()))
    {}
    virtual ~scene_element_t() noexcept = default;

    [[nodiscard]] const std::string& get_id() const noexcept { return id ? *id : empty_id; }

public:
    static std::shared_ptr<scene_element_t> load(std::string id, 
                                                 scene::loader::loader_t* loader, 
                                                 const scene::loader::node_t& node, 
                                                 const wt::wt_context_t &context);

    [[nodiscard]] virtual element::info_t description() const = 0;
};

}
