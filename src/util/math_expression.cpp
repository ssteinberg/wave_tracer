/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <format>
#include <wt/util/math_expression.hpp>
#include <tinyexpr.h>


using namespace wt;

struct compiled_math_expression_t::compiled_math_expression_data_t{
    te_parser tep;
};

thread_local const std::vector<f_t>* variable_lookup_variables = nullptr;
f_t expr_variable_lookup(int idx) {
    if (!variable_lookup_variables || variable_lookup_variables->size()<=idx)
        throw std::runtime_error("(compiled_math_expression_t) variable not found");
    return (*variable_lookup_variables)[idx];
}

compiled_math_expression_t::compiled_math_expression_t(std::string expr, const std::vector<std::string>& variables) 
    : expression(std::move(expr)),
      data(new compiled_math_expression_data_t)
{
    auto& d = *data;

    // bind variables
    {
        std::set<te_variable> vars;
        for (auto varidx=0ul; varidx<variables.size(); ++varidx)
            vars.emplace(variables[varidx], te_varfun{ expr_variable_lookup, varidx });
        d.tep.set_variables_and_functions(std::move(vars));
    }

    // compile
    if (!d.tep.compile(this->expression)) {
        throw std::format_error(std::string{ "failed parsing math expression (at " } + 
                std::format("{:d}",d.tep.get_last_error_position()) + ") " + d.tep.get_last_error_message());
    }
}
compiled_math_expression_t::~compiled_math_expression_t() {
    if (data)
        delete data;
}

compiled_math_expression_t::compiled_math_expression_t(compiled_math_expression_t &&o) noexcept
    : expression(std::move(o.expression)),
      data(o.data)
{
    o.data = nullptr;
}

std::size_t compiled_math_expression_t::get_variable_count() const noexcept {
    return data->tep.get_variables_and_functions().size();
}

f_t compiled_math_expression_t::eval(const std::vector<f_t>& vars) const {
    try {
        variable_lookup_variables = &vars;
        const auto ret = data->tep.evaluate();
        variable_lookup_variables = nullptr;

        return ret;
    } catch(const std::runtime_error& er) {
        variable_lookup_variables = nullptr;
        throw er;
    }
}


f_t wt::detail::evaluate_math_expression(const std::string& expr) {
    te_parser tep;

    auto result = tep.evaluate(expr.c_str());
    if (tep.success()) return result;

    throw std::format_error(std::string{ "failed parsing math expression (at " } + std::format("{:d}",tep.get_last_error_position()) + ") " + tep.get_last_error_message());
}
