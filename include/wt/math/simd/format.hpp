/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/format.hpp>
#include <wt/util/concepts.hpp>
#include "wide_vector.hpp"

#include <format>


template <std::size_t Width, std::size_t N, wt::Quantity Q>
struct std::formatter<wt::wide_vector<Width,N,Q>> : std::formatter<Q> {
    using V = wt::wide_vector<Width,N,Q>;

    auto format(const V& v, std::format_context& ctx) const {
        const auto open_bracket  = "{";
        const auto close_bracket = "}";
        const auto sep = "; ";

        std::string temp;
        for (auto i=0ul;i<Width-1;++i) 
            std::format_to(std::back_inserter(temp), "{}{}", v.read(i), sep);
        std::format_to(std::back_inserter(temp), "{}", v.read(Width-1));

        return std::format_to(ctx.out(), "{}{}{}",
                    open_bracket, temp, close_bracket);
    }
};
