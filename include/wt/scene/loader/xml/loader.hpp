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
#include <map>

#include <istream>
#include <optional>
#include <memory>

#include <wt/wt_context.hpp>
#include <wt/util/logger/logger.hpp>

#include <wt/scene/loader/loader.hpp>
#include <wt/scene/loader/node.hpp>


namespace pugi { class xml_node; }


namespace wt::scene::loader::xml {

struct xml_data_source_t;

/**
 * @brief Node for an XML data source.
 */
class xml_node_t final : public wt::scene::loader::node_t {
    std::string nname, xml_path;
    std::vector<std::unique_ptr<node_t>> cs;
    std::map<std::string,std::string> attribs;

    std::size_t node_offset;
    const xml_data_source_t* data_source;

public:
    xml_node_t(const pugi::xml_node& xmlnode, const xml_data_source_t* data_source) noexcept;
    ~xml_node_t() noexcept = default;


    [[nodiscard]] const auto* get_data_source() const noexcept { return data_source; }

    /**
     * @brief Node type. For example, XML nodes should all return the same type (e.g., "XML").
     */
    [[nodiscard]] const std::string& type() const noexcept override {
        static auto type = std::string{ "XML" };
        return type;
    }

    /**
     * @brief Node path.
     */
    [[nodiscard]] const std::string& path() const noexcept override {
        return xml_path;
    }

    /**
     * @brief Node name.
     */
    [[nodiscard]] const std::string& name() const noexcept override {
        return nname;
    }

    [[nodiscard]] bool has_attrib(const std::string& attribute) const noexcept override {
        return attribs.contains(attribute);
    }
    /**
     * @brief Accesses an attribute by name.
     */
    [[nodiscard]] const std::string& operator[](const std::string& attribute) const noexcept override {
        const auto it = attribs.find(attribute);
        if (it != attribs.end()) return it->second;

        static std::string empty = {};
        return empty;
    }
    /**
     * @brief List of attributes.
     */
    [[nodiscard]] const std::map<std::string, std::string>& attributes() const noexcept override {
        return attribs;
    }

    /**
     * @brief List of all children nodes of name ``name``.
     */
    [[nodiscard]] std::vector<const node_t*> children(const std::string& name) const noexcept override {
        std::vector<const node_t*> ret;
        for (const auto& c : cs)
            if (c->name() == name)
                ret.emplace_back(c.get());
        return ret;
    }
    /**
     * @brief List of all children nodes.
     */
    [[nodiscard]] const std::vector<std::unique_ptr<node_t>>& children() const noexcept override {
        return cs;
    }
    /**
     * @brief Moves all children out of this node. Destructive.
     */
    [[nodiscard]] std::vector<std::unique_ptr<node_t>> extract_children() && noexcept override {
        std::vector<std::unique_ptr<node_t>> ret;
        ret.reserve(cs.size());
        for (auto& c : cs)
            ret.emplace_back(std::move(c));
        cs.clear();
        return ret;
    }

    /**
     * @brief Sets or updates attribute value.
     */
    bool set_attribute(const std::string& name, const std::string& value) noexcept override {
        const auto it = attribs.find(name);
        if (it == attribs.end()) return false;

        it->second = value;
        return true;
    }

    /**
     * @brief Insert a child node.
     */
    bool add_child(std::unique_ptr<node_t> child) noexcept override {
        cs.emplace_back(std::move(child));
        return true;
    }
    /**
     * @brief Replaces a child node ``child`` with other nodes.
     */
    bool replace_child(const node_t& child, std::vector<std::unique_ptr<node_t>> nodes) noexcept override {
        for (auto it=cs.begin(); it!=cs.end(); ++it) {
            if (it->get()==&child) {
                it = cs.erase(it);
                cs.insert(it,
                          std::make_move_iterator(nodes.begin()),
                          std::make_move_iterator(nodes.end()));
                return true;
            }
        }
        return false;
    }
    /**
     * @brief Remove the node from this node's children. Does nothing if ``node`` is not a child node.
     */
    bool erase_child(const node_t& node) noexcept override {
        for (auto it=cs.begin(); it!=cs.end(); ++it) {
            if (it->get()==&node) {
                it = cs.erase(it);
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] inline auto get_node_offset_in_xmlfile() const noexcept { return node_offset; }
    [[nodiscard]] inline auto* get_node_data_source() const noexcept { return data_source; }
};


/**
 * @brief Scene loader from an XML data source.
 */
class xml_loader_t final : public loader_t {
public:
    xml_loader_t(std::string name, 
                 const wt_context_t &ctx,
                 std::istream& xml,
                 const defaults_defines_t& defines = {},
                 std::optional<progress_callback_t> callbacks = {});
    virtual ~xml_loader_t();
 
    [[nodiscard]] inline std::string node_description(const node_t& node) const noexcept override;

private:
    bool merge_includes(const wt_context_t &ctx, node_t& node);

private:
    std::vector<std::unique_ptr<xml_data_source_t>> data_sources;
};


}