/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/common.hpp>

namespace wt::sensor {

template <typename PixelT, typename WeightT>
struct sensor_element_t {
    PixelT value;
    WeightT weight;

    inline auto pixel_value() const noexcept {
        return weight>0 ? value/weight : PixelT{};
    }

    inline auto& operator+=(const sensor_element_t& o) noexcept {
        value += o.value;
        weight += o.weight;
        return *this;
    }
    inline auto operator+(const sensor_element_t& o) const noexcept {
        return sensor_element_t{ 
            value+o.value,
            weight+o.weight
        };
    }
    inline auto& operator*=(const WeightT scale) noexcept {
        value *= scale;
        weight *= scale;
        return *this;
    }
    inline auto operator*(const WeightT scale) const noexcept {
        return sensor_element_t{ 
            value*scale,
            weight*scale
        };
    }

    template <std::constructible_from<PixelT> NewT, std::constructible_from<WeightT> NewW>
    inline explicit operator sensor_element_t<NewT, NewW>() const noexcept {
        sensor_element_t<NewT,NewW> ret;
        ret.value  = NewT{ value };
        ret.weight = NewW{ weight };
        return ret;
    }
};

}
