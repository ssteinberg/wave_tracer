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
#include <string_view>

#include <regex>

#include <type_traits>
#include <format>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>

#include <wt/util/concepts.hpp>
#include <wt/util/math_expression.hpp>

#include <wt/util/format/utils.hpp>


namespace wt {

inline auto stob_strict(const std::string& str) {
    return parse_expression<bool>(str);
}
inline auto stoff_strict(const std::string& str) {
    return parse_expression<float>(str);
}
inline auto stofd_strict(const std::string& str) {
    return parse_expression<double>(str);
}
inline auto stofld_strict(const std::string& str) {
    return parse_expression<long double>(str);
}
template <typename T=f_t>
inline auto stof_strict(const std::string& str) {
    return parse_expression<T>(str);
}
inline auto stoi_strict(const std::string& str) {
    return parse_expression<int>(str);
}
inline auto stol_strict(const std::string& str) {
    return parse_expression<long>(str);
}
inline auto stoll_strict(const std::string& str) {
    return parse_expression<long long>(str);
}
inline auto stoul_strict(const std::string& str) {
    return parse_expression<unsigned long>(str);
}
inline auto stoull_strict(const std::string& str) {
    return parse_expression<unsigned long long>(str);
}
inline auto stob_strict(const std::string& str, bool default_when_empty) {
    return str.length() ? stob_strict(str) : default_when_empty;
}
inline auto stof_strict(const std::string& str, f_t default_when_empty) {
    return str.length() ? stof_strict(str) : default_when_empty;
}
inline auto stoi_strict(const std::string& str, int default_when_empty) {
    return str.length() ? stoi_strict(str) : default_when_empty;
}
inline auto stol_strict(const std::string& str, long default_when_empty) {
    return str.length() ? stol_strict(str) : default_when_empty;
}
inline auto stoll_strict(const std::string& str, long long default_when_empty) {
    return str.length() ? stoll_strict(str) : default_when_empty;
}
inline auto stoul_strict(const std::string& str, unsigned long default_when_empty) {
    return str.length() ? stoul_strict(str) : default_when_empty;
}
inline auto stoull_strict(const std::string& str, unsigned long long default_when_empty) {
    return str.length() ? stoull_strict(str) : default_when_empty;
}

template <typename T>
    requires std::is_same_v<T, float> ||
             std::is_same_v<T, double> ||
             std::is_same_v<T, long double> ||
             std::is_same_v<T, bool> ||
             std::is_same_v<T, int> ||
             std::is_same_v<T, long> ||
             std::is_same_v<T, long long> ||
             std::is_same_v<T, unsigned long> ||
             std::is_same_v<T, unsigned long long>
inline auto stonum_strict(const std::string& str) {
    if constexpr (std::is_same_v<T, float>)               return stoff_strict(str);
    if constexpr (std::is_same_v<T, double>)              return stofd_strict(str);
    if constexpr (std::is_same_v<T, long double>)         return stofld_strict(str);
    if constexpr (std::is_same_v<T, bool>)                return stob_strict(str);
    if constexpr (std::is_same_v<T, int>)                 return stoi_strict(str);
    if constexpr (std::is_same_v<T, long>)                return stol_strict(str);
    if constexpr (std::is_same_v<T, long long>)           return stoll_strict(str);
    if constexpr (std::is_same_v<T, unsigned long>)       return stoul_strict(str);
    if constexpr (std::is_same_v<T, unsigned long long>)  return stoull_strict(str);
}

/**
 * @brief String to complex number. String must be "<a>" or "(<a>)" or "(<a>,<b>i)" where "<a>" and "<b>" are formatted floating-point numbers.
 */
inline auto parse_complex_strict(const std::string& str) {
    f_t r,i=0;

    if (str[0]!='(') {
        return c_t{ stof_strict(str), 0 };
    }
    if (str.size()<3 || str[str.size()-1]!=')' || str[str.size()-2]!='i')
        throw std::format_error("(stoc) malformed string");
    const auto inner = format::trim(str.substr(1, str.size()-3));

    const auto end = inner[0]=='(' ? format::find_closing_bracket(inner) : inner.find(',');
    if (end==std::string::npos || inner[end]!=',')
        throw std::format_error("(stoc) malformed string");

    const auto real_part = format::trim(inner.substr(0,end));
    const auto imag_part = format::trim(inner.substr(end+1));
    r = stof_strict(std::string{ real_part });
    i = stof_strict(std::string{ imag_part });

    return c_t{r,i};
}

/**
 * @brief String to 4x4 matrix.
 */
inline mat4_t parse_matrix4(const std::string& str) {
    mat4_t m{};
    std::stringstream ss(str);
    std::string expr;
    for (std::size_t r=0;r<4;++r)
    for (std::size_t c=0;c<4;++c) {
        std::getline(ss, expr, ',');
        m[c][r] = stof_strict(format::trim(expr));
        if (ss.peek() == ',')
            ss.ignore();
    }
    return m;
}
/**
 * @brief String to 3x3 matrix.
 */
inline mat3_t parse_matrix3(const std::string& str) {
    mat3_t m{};
    std::stringstream ss(str);
    std::string expr;
    for (std::size_t r=0;r<3;++r)
    for (std::size_t c=0;c<3;++c) {
        std::getline(ss, expr, ',');
        m[c][r] = stof_strict(format::trim(expr));
        if (ss.peek() == ',')
            ss.ignore();
    }
    return m;
}
/**
 * @brief String to 2x2 matrix.
 */
inline mat2_t parse_matrix2(const std::string& str) {
    mat2_t m{};
    std::stringstream ss(str);
    std::string expr;
    for (std::size_t r=0;r<2;++r)
    for (std::size_t c=0;c<2;++c) {
        std::getline(ss, expr, ',');
        m[c][r] = stof_strict(format::trim(expr));
        if (ss.peek() == ',')
            ss.ignore();
    }
    return m;
}
/**
 * @brief String to vector.
 */
template <FloatingPoint Fp=f_t>
inline auto parse_vec4(const std::string& str) {
    vec4<Fp> v{};
    std::stringstream ss(str);
    std::string expr;
    for (std::size_t i=0;i<4;++i) {
        std::getline(ss, expr, ',');
        v[i] = stonum_strict<Fp>(format::trim(expr));
        if (ss.peek() == ',')
            ss.ignore();
    }
    return v;
}
/**
 * @brief String to vector.
 */
template <FloatingPoint Fp=f_t>
inline auto parse_vec3(const std::string& str) {
    vec3<Fp> v{};
    std::stringstream ss(str);
    std::string expr;
    for (std::size_t i=0;i<3;++i) {
        std::getline(ss, expr, ',');
        v[i] = stonum_strict<Fp>(format::trim(expr));
        if (ss.peek() == ',')
            ss.ignore();
    }
    return v;
}
/**
 * @brief String to vector.
 */
template <FloatingPoint Fp=f_t>
inline auto parse_vec2(const std::string& str) {
    vec2<Fp> v{};
    std::stringstream ss(str);
    std::string expr;
    for (std::size_t i=0;i<2;++i) {
        std::getline(ss, expr, ',');
        v[i] = stonum_strict<Fp>(format::trim(expr));
        if (ss.peek() == ',')
            ss.ignore();
    }
    return v;
}

/**
 * @brief String to range.
 */
template <FloatingPoint T, range_inclusiveness_e inc>
inline void parse_range(const std::string& str, range_t<T,inc>& out) {
    const auto sep = str.find("..");
    if (sep == std::string::npos)
        throw std::format_error("(parse_range) malformed range expression, expected '<min>..<max>'");

    const auto rmin = std::string_view(str).substr(0,sep);
    const auto rmax = std::string_view(str).substr(sep+2);

    range_t<T,inc> r;
    r.min = stof_strict<T>(format::trim(rmin));
    r.max = stof_strict<T>(format::trim(rmax));
    out = r;
}


inline auto parse_hostname_and_port(const std::string& str) {
    const std::regex regex{ "^(.*?):([0-9]+)$" };
    std::smatch match;
    if (!std::regex_match(str, match, regex) ||
        match.size() != 3)
        throw std::format_error("(parse_hostname_and_port) malformed expression");

    const auto host = match[1].str();
    const auto port = std::stol(match[2].str());

    return std::make_pair(host,port);
}

}
