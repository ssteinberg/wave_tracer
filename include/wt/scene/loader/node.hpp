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
#include <map>
#include <vector>
#include <string>
#include <ranges>

namespace wt::scene::loader {

/** 
 * @brief Scene data source node interface.
 */
class node_t {
public:
    virtual ~node_t() noexcept = default;

    [[nodiscard]] std::partial_ordering operator<=>(const node_t& o) const noexcept {
        if (type() != o.type()) return std::partial_ordering::unordered;
        return path() <=> o.path();
    }

    /**
     * @brief Node type. For example, XML nodes should all return the same type (e.g., "XML").
     */
    [[nodiscard]] virtual const std::string& type() const noexcept = 0;

    /**
     * @brief Node path.
     */
    [[nodiscard]] virtual const std::string& path() const noexcept = 0;

    /**
     * @brief Node name.
     */
    [[nodiscard]] virtual const std::string& name() const noexcept = 0;

    [[nodiscard]] virtual bool has_attrib(const std::string& attribute) const noexcept = 0;

    /**
     * @brief Accesses an attribute by name.
     */
    [[nodiscard]] virtual const std::string& operator[](const std::string& attribute) const noexcept = 0;
    /**
     * @brief List of attributes.
     */
    [[nodiscard]] virtual const std::map<std::string, std::string>& attributes() const noexcept = 0;

    /**
     * @brief Sets or updates attribute value.
     */
    virtual bool set_attribute(const std::string& name, const std::string& value) noexcept = 0;

    /**
     * @brief List of all children nodes of name ``name``.
     */
    [[nodiscard]] virtual std::vector<const node_t*> children(const std::string& name) const noexcept = 0;
    /**
     * @brief List of all children nodes.
     */
    [[nodiscard]] virtual const std::vector<std::unique_ptr<node_t>>& children() const noexcept = 0;
    /**
     * @brief Moves all children out of this node. Destructive.
     */
    [[nodiscard]] virtual std::vector<std::unique_ptr<node_t>> extract_children() && noexcept = 0;

    /**
     * @brief Insert a child node.
     */
    virtual bool add_child(std::unique_ptr<node_t> child) noexcept = 0;
    /**
     * @brief Replaces a child node ``child`` with other nodes.
     */
    virtual bool replace_child(const node_t& child, std::vector<std::unique_ptr<node_t>> nodes) noexcept = 0;
    /**
     * @brief Remove the node ``node`` from this node's children. Does nothing if ``node`` is not a child node.
     * @return TRUE if the child node was found and removed, FALSE otherwise.
     */
    virtual bool erase_child(const node_t& node) noexcept = 0;

    /**
     * @brief A view of all children as const references.
     */
    [[nodiscard]] inline auto children_view() const noexcept {
        return this->children() | std::views::transform([](const auto& p) -> const auto& { return *p; });
    }
    /**
     * @brief A view of all children as references.
     */
    [[nodiscard]] inline auto children_view() noexcept {
        return this->children() | std::views::transform([](auto& p) -> auto& { return *p; });
    }
};

}
