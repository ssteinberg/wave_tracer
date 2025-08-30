/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <fstream>
#include <unordered_map>

#include <wt/scene/loader/xml/loader.hpp>
#include <wt/util/logger/logger.hpp>

#include <pugixml.hpp>

using namespace wt;
using namespace wt::scene::loader::xml;

using node_t = wt::scene::loader::node_t;


struct wt::scene::loader::xml::xml_data_source_t {
    /** @brief For external data sources. E.g., the file path for included xml docs. */
    std::optional<std::string> source_name;

    pugi::xml_document doc;
    std::unique_ptr<xml_node_t> root;

    std::unordered_map<const node_t*, std::size_t> node_to_linenumber_db;
};

struct xml_load_result_t {
    std::unique_ptr<xml_data_source_t> data_source;

    bool success;
    std::string error;
};


xml_node_t::xml_node_t(const pugi::xml_node& xmlnode, const xml_data_source_t* data_source) noexcept {
    nname = xmlnode.name();
    xml_path = xmlnode.path();
    if (data_source->source_name)
        xml_path = "\"" + *data_source->source_name + "\"/" + xml_path;

    node_offset = xmlnode.offset_debug();
    this->data_source = data_source;
    
    for (const auto& c : xmlnode.children())
        cs.emplace_back(std::make_unique<xml_node_t>(c, data_source));
    for (const auto& a : xmlnode.attributes())
        attribs.emplace(a.name(), std::string{ a.value() });
}


void extract_node_offsets(const xml_node_t& node, 
                          std::vector<std::pair<const node_t*, std::size_t>>& offsets) noexcept {
    offsets.emplace_back(&node, node.get_node_offset_in_xmlfile());
    for (const auto& child : node.children())
        extract_node_offsets(*dynamic_cast<const xml_node_t*>(child.get()), offsets);
}
inline auto create_node_to_linenumber_db(const xml_node_t& root, const std::string& xml_file) noexcept {
    // recursively extract offsets
    std::vector<std::pair<const node_t*, std::size_t>> offsets;
    extract_node_offsets(root, offsets);

    std::unordered_map<const node_t*, std::size_t> db;

    std::size_t line=1, i=0ul, prev_offset=0;
    for (const auto& e : offsets) {
        const auto offset = e.second;
        if (offset<prev_offset) {
            // generally, this shouldn't happen: we expect offsets to be given in an ascending order
            // if this does, restart parsing through the xml_file
            line=1; i=0ul; prev_offset=0;
        }

        // find line
        for (; i<offset && i<xml_file.length(); ++i)
            if (xml_file[i]=='\n') ++line;
        db.emplace(e.first, line);
    }

    return db;
}


[[nodiscard]] inline xml_load_result_t read_xml(std::istream& xml) {
    xml_load_result_t ret;

    // read XML
    auto xml_file = std::string{ std::istreambuf_iterator<char>(xml),
                                 std::istreambuf_iterator<char>() };

    auto data_source = std::make_unique<xml_data_source_t>();

    // load scene XML
    const auto xml_load_result = data_source->doc.load_string(xml_file.c_str());
    if (!xml_load_result) {
        std::size_t line=1;
        for (auto i=0ul; i<xml_load_result.offset && i<xml_file.length(); ++i)
            if (xml_file[i]=='\n') ++line;

        ret.success = false;
        ret.error = "line " + std::to_string(line) + ": " + std::string{ xml_load_result.description() };
        
        return ret;
    }

    // create root xml node
    data_source->root = std::make_unique<xml_node_t>(data_source->doc, data_source.get());
    // create the node-to-linenumber database
    data_source->node_to_linenumber_db = create_node_to_linenumber_db(*data_source->root, xml_file);

    ret.success = true;
    ret.data_source = std::move(data_source);

    return ret;
}


xml_loader_t::xml_loader_t(
        std::string name, 
        const wt_context_t &ctx,
        std::istream& xmlis,
        const defaults_defines_t& user_defines,
        std::optional<progress_callback_t> callbacks)
    : loader_t(std::move(name),ctx,std::move(callbacks))
{
    // read XML
    xml_data_source_t* ds = nullptr;
    {
        auto xml = read_xml(xmlis);
        if (!xml.success) {
            logger::cerr(verbosity_e::important) << "(xml loader) " << xml.error << '\n';
            this->set_fail();
            return;
        }

        ds = xml.data_source.get();
        this->data_sources.emplace_back(std::move(xml.data_source));
    }
    assert(ds);

    // find "scene" root node
    xml_node_t* xml_scene_node = nullptr;
    for (auto& c : ds->root->children_view()) {
        if (c.name() != "scene") {
            logger::cerr(verbosity_e::important) << R"((xml loader) expected "scene" root node.)" << '\n';
            this->set_fail();
            return;
        } else if (xml_scene_node) {
            logger::cerr(verbosity_e::important) << R"((xml loader) multiple "scene" root nodes defined.)" << '\n';
            this->set_fail();
            return;
        }
        xml_scene_node = reinterpret_cast<xml_node_t*>(&c);
        assert(xml_scene_node);
    }
    if (!xml_scene_node) {
        logger::cerr(verbosity_e::important) << R"((xml loader) "scene" root node not found.)" << '\n';
        this->set_fail();
        return;
    }

    // merge external included nodes
    if (!merge_includes(ctx, *xml_scene_node))
        return;

    // start load
    this->load(xml_scene_node, user_defines);
}

xml_loader_t::~xml_loader_t() = default;

bool xml_loader_t::merge_includes(const wt_context_t &ctx, node_t& node) {
    std::map<std::pair<node_t*, const node_t*>, xml_data_source_t*> copy;

    for (auto& c : node.children_view()) {
        if (std::string{ c.name() } == "include") {
            if (!c.has_attrib("path")) {
                // malformed include?
                logger::cerr(verbosity_e::important) << R"((xml loader) "include" node has no path attribute.)" << '\n';
                continue;
            }

            // load external XML and include it
            const auto path = std::filesystem::path{ c["path"] };
            const auto resolved_path = ctx.resolve_path(path);

            if (!resolved_path) {
                // include not found
                logger::cerr(verbosity_e::important) << "(xml loader) path \"" << path << R"(" of "include" node not found.)" << '\n';
                this->set_fail();
                return false;
            }

            auto fis = std::ifstream{ *resolved_path };
            auto xml = read_xml(fis);
            // data source name
            xml.data_source->source_name = path.string();
            
            if (!xml.success) {
                // include parsing failed
                logger::cerr(verbosity_e::important) << "(xml loader) error in include \"" << path << R"(": )" << xml.error << '\n';
                this->set_fail();
                return false;
            }

            // add data source
            this->data_sources.emplace_back(std::move(xml.data_source));
            // queue copy
            copy.emplace(std::make_pair(&node, &c), this->data_sources.back().get());
        }

        if (!merge_includes(ctx, c))
            return false;
    }

    // copy
    for (auto& c : copy) {
        auto* parent = c.first.first;
        auto* child  = c.first.second;
        auto& src    = *c.second->root;
        if (!parent->replace_child(*child,
                                   std::move(src).extract_children())) {
            logger::cerr(verbosity_e::important) << "(xml loader) error merging nodes from input \"" << *c.second->source_name << "\"" << '\n';
            this->set_fail();
            return false;
        }
        c.second->root = nullptr;
    }

    return true;
}

[[nodiscard]] std::string xml_loader_t::node_description(const node_t& node) const noexcept {
    const auto* xml_node = dynamic_cast<const xml_node_t*>(&node);
    assert(xml_node);
    if (!xml_node)
        return {};

    const auto* ds = xml_node->get_node_data_source();

    // query data source to see if we have line number info
    std::optional<std::size_t> ln;
    {
        const auto it = ds->node_to_linenumber_db.find(xml_node);
        if (it!=ds->node_to_linenumber_db.end())
            ln = it->second;
    }

    // data source name
    const auto& name = ds->source_name;

    if (!name && !ln) return "";
    return "[" + (name ? *name+" " : "") + "line " + std::format("{:d}",*ln) + " — " + node.name() + "] ";
}
