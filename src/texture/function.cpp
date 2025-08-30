/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/scene/loader/node.hpp>
#include <wt/scene/element/attributes.hpp>
#include <wt/scene/loader/node_readers.hpp>
#include <wt/wt_context.hpp>

#include <wt/texture/function.hpp>

#include <wt/math/common.hpp>
#include <wt/util/math_expression.hpp>
#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace texture;


scene::element::info_t function_t::description() const {
    using namespace scene::element;

    std::vector<attribute_ptr> texs;
    for (const auto& t : this->texs)
        texs.emplace_back(attributes::make_element(t.get()));

    return info_for_scene_element(*this, "function", {
        { "function", attributes::make_string(this->func_description) },
        { "textures",  attributes::make_array(std::move(texs)) },
    });
}

std::unique_ptr<texture_t> function_t::load(
        std::string id, 
        scene::loader::loader_t* loader, 
        const scene::loader::node_t& node, 
        const wt::wt_context_t &context) {
    std::vector<std::shared_ptr<texture_t>> texs;
    std::vector<std::string> varnames;

    for (auto& item : node.children_view()) {
    try {
        std::shared_ptr<texture_t> tex;
        if (scene::loader::load_texture_element(item, tex, loader, context)) {
            const auto name = item["name"];
            if (name.empty())
                throw scene_loading_exception_t("(function texture loader) Nested texture must be given a name", item);

            varnames.emplace_back(name);
            texs.emplace_back(std::move(tex));
        }
        else if (item.name()!="function")
            logger::cwarn()
                << loader->node_description(item)
                << "(function texture loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(function texture loader) " + std::string{ exp.what() }, item);
    }
    }

    // add wavenumber as additional variable
    varnames.emplace_back("u");
    varnames.emplace_back("v");
    varnames.emplace_back("k");

    std::optional<compiled_math_expression_t> expr;
    try {
        // function can be given inline as function="<function>", used as a shorthand, or as an attribute.
        const bool has_shorthand_function = node["function"][0] != '\0';
        if (has_shorthand_function)
            scene::loader::load_function_from_attribute(node, "function", expr, varnames);

        const auto& c = node.children("function");
        if (c.size()>1) {
            logger::cwarn()
                << loader->node_description(node)
                << "(function texture loader) multiple nested functions provided." << '\n';
        }
        if (!c.empty())
            scene::loader::load_function(*c.front(), expr, varnames);
    } catch(const std::format_error& exp) {
        throw scene_loading_exception_t("(function texture loader) " + std::string{ exp.what() }, node);
    }
    
    if (!expr)
        throw scene_loading_exception_t("(function texture loader) No function 'function' provided", node);

    auto func_description = expr->description();

    // wrapper around user-supplied function
    function_t::func_t func = [expr=std::move(*expr)](
            const function_t::tex_container_t& texs,
            const texture_query_t& query,
            const vec2_t& uv,
            const wavenumber_t k) noexcept -> f_t {
        // set vars
        std::vector<f_t> vars;
        vars.resize(expr.get_variable_count());
        for (auto i=0ul; i<texs.size(); ++i)
            vars[i] = texs[i]->f(query).x;
        vars[texs.size()+0] = (f_t)(query.uv.x);
        vars[texs.size()+1] = (f_t)(query.uv.y);
        vars[texs.size()+2] = (f_t)(query.k * u::mm);

        return expr.eval(vars);
    };

    auto ret = std::make_unique<function_t>(
        std::move(id),
        std::move(texs),
        std::move(func),
        std::move(func_description)
    );

    for (const auto& t : ret->texs)
        loader->register_resource_dependency(ret.get(), t.get());

    return ret;
}
