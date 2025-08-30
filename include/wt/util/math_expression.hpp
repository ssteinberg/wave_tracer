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

#include <wt/util/concepts.hpp>
#include <wt/math/common.hpp>

namespace wt {

/**
 * @brief Compiles a math expression defined via a string with free variable, and enables efficient run-time evaluation.
 */
struct compiled_math_expression_t {
    struct compiled_math_expression_data_t;

private:
    std::string expression;
    compiled_math_expression_data_t* data = nullptr;

public:
    compiled_math_expression_t() = delete;
    /**
     * @brief Constructs a new compiled math expression.
     *        Throws if compilation fails.
     * 
     * @param expr the math expression
     * @param variables free variables in `expr`
     */
    compiled_math_expression_t(std::string expr, const std::vector<std::string>& variables);
    ~compiled_math_expression_t();

    compiled_math_expression_t(compiled_math_expression_t&&) noexcept;

    [[nodiscard]] const auto& description() const noexcept { return expression; }
    [[nodiscard]] std::size_t get_variable_count() const noexcept;

    /**
     * @brief Evaluates the expression. Variables and expression result are evaluated with type `f_t`.
     * 
     * @param vars list of values for the free variables; must be of same length as the `variables` parameter for the ctor
     */
    [[nodiscard]] f_t eval(const std::vector<f_t>& vars = {}) const;
};


namespace detail {

extern f_t evaluate_math_expression(const std::string& expr);

}

/**
 * @brief Parses a math expression defined as the string `expr` and returns the evaluated result.
 * 
 * @tparam type of result, must be either a Numeric or a bool
 * @return R the evaluated result
 */
template <NumericOrBool R>
inline R parse_expression(const std::string& expr) {
    using lim = limits<R>;

    const auto result = detail::evaluate_math_expression(expr.c_str());

    // successful parse
    if (std::is_same_v<R,bool>) {
        if (result==0) return false;
        if (result==1) return true;
        throw std::format_error("math expression: expected boolean result (got '" + std::format("{}",result) + "')");
    }
    else if (std::is_integral_v<R>) {
        // narrowing conversion

        // TODO: correct overflow/underflow detection
        if ((decltype(result))lim::max() < result)
            throw std::format_error("math expression: overflow");
        if ((decltype(result))lim::min() > result)
            throw std::format_error("math expression: underflow");

        return static_cast<R>(result);
    }

    return static_cast<R>(result);
}

}
