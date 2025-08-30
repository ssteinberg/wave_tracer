/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <algorithm>
#include <vector>

#include <cassert>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>
#include "distribution1d.hpp"

namespace wt {

/**
 * @brief Continuos piecewise-linear distribution
 */
class piecewise_linear_distribution_t final : public distribution1d_t {
private:
    std::vector<vec2_t> bins;
    std::vector<f_t> dcdf;
    f_t sum, norm;

private:
    [[nodiscard]] inline f_t value_impl(f_t x, decltype(bins)::const_iterator it) const noexcept {
        if (it==bins.end())
            return 0;
        if (it==bins.begin() || it->x==x)
            return it->x==x ? it->y : 0;

        const auto& a = *std::prev(it);
        const auto& b = *it;
        const auto dx = b.x-a.x;
        assert(dx>0 && x>=a.x && b.x>x);

        return m::mix(a.y,b.y,(x-a.x)/dx);
    }

public:
    /**
     * @brief Construct a piecewise_linear_distribution_t
     * @param values non negative values, with x in strictly ascending order
     */
    explicit piecewise_linear_distribution_t(std::vector<vec2_t> values) 
        : bins(std::move(values))
    {
        assert(bins.size()>1);
        if (bins.size()<=1)
            return;

        // accumulate
        dcdf.resize(bins.size());
        dcdf[0]=0;
        for (std::size_t i=1;i<bins.size();++i) {
            auto dx = bins[i].x - bins[i-1].x;
            assert(bins[i].x>=0 && bins[i-1].x>=0);
            if (dx<0) {
                bins[i].x = bins[i-1].x;
                dx = 0;
            }
            dcdf[i] = dcdf[i-1] + dx*(bins[i].y + bins[i-1].y)/2;
        }
        
        // normalize
        sum = dcdf.back();
        const auto recp_sum = sum>0 ? f_t(1)/sum : f_t(0);
        std::ranges::transform(dcdf, dcdf.begin(), 
                               [n=recp_sum](const auto c) { return c*n; });
        
        norm = recp_sum;
    }

    piecewise_linear_distribution_t(piecewise_linear_distribution_t&&) noexcept = default;
    piecewise_linear_distribution_t(const piecewise_linear_distribution_t&) noexcept = default;
    piecewise_linear_distribution_t& operator=(piecewise_linear_distribution_t&&) noexcept = default;
    piecewise_linear_distribution_t& operator=(const piecewise_linear_distribution_t&) noexcept = default;

    [[nodiscard]] std::unique_ptr<distribution1d_t> clone() const override {
        return std::make_unique<piecewise_linear_distribution_t>(*this);
    }

    [[nodiscard]] inline auto total() const noexcept { return sum; }
    [[nodiscard]] inline auto range() const noexcept { return range_t{ bins.front().x, bins.back().x }; }

    [[nodiscard]] inline auto& operator[](std::size_t idx) { return bins[idx]; }
    [[nodiscard]] inline const auto& operator[](std::size_t idx) const { return bins[idx]; }

    [[nodiscard]] inline auto size() const noexcept { return bins.size(); }

    [[nodiscard]] inline auto begin() noexcept { return bins.begin(); }
    [[nodiscard]] inline auto end() noexcept { return bins.end(); }
    [[nodiscard]] inline auto begin() const noexcept { return bins.begin(); }
    [[nodiscard]] inline auto end() const noexcept { return bins.end(); }

    [[nodiscard]] inline auto& front() noexcept { return bins.front(); }
    [[nodiscard]] inline auto& back() noexcept { return bins.back(); }
    [[nodiscard]] inline const auto& front() const noexcept { return bins.front(); }
    [[nodiscard]] inline const auto& back() const noexcept { return bins.back(); }

    [[nodiscard]] inline auto cbegin() const noexcept { return bins.cbegin(); }
    [[nodiscard]] inline auto cend() const noexcept { return bins.cend(); }
    [[nodiscard]] inline auto crbegin() const noexcept { return bins.crbegin(); }
    [[nodiscard]] inline auto crend() const noexcept { return bins.crend(); }

    [[nodiscard]] inline auto rbegin() noexcept { return bins.rbegin(); }
    [[nodiscard]] inline auto rend() noexcept { return bins.rend(); }
    [[nodiscard]] inline auto rbegin() const noexcept { return bins.rbegin(); }
    [[nodiscard]] inline auto rend() const noexcept { return bins.rend(); }

    [[nodiscard]] inline auto operator+(const piecewise_linear_distribution_t& d) const noexcept {
        std::vector<vec2_t> v;
        v.reserve(bins.size());

        // TODO: handle edges correctly
        auto it1 = bins.cbegin(), it2 = d.bins.cbegin();
        while (it1!=bins.cend() || it2!=d.bins.cend()) {
            if (it1!=bins.cend() && (it2==d.bins.cend() || it1->x<=it2->x)) {
                const auto y2 = d.value_impl(it1->x, it2);
                v.emplace_back(it1->x, it1->y + y2);
                if (it2!=d.bins.cend() && it2->x==it1->x)
                    ++it2;
                ++it1;
            } else {
                const auto y1 = value_impl(it2->x, it1);
                v.emplace_back(it2->x, y1 + it2->y);
                if (it1!=bins.cend() && it2->x==it1->x)
                    ++it1;
                ++it2;
            }
        }

        return piecewise_linear_distribution_t{ std::move(v) };
    }

    /**
     * @brief Value at position
     * @param x position
     * @return Piece-wise linearly interpolated value
     */
    [[nodiscard]] inline f_t value(f_t x) const noexcept {
        const auto it = std::lower_bound(bins.begin(), bins.end(), x, 
                                         [](const auto& bin, const auto x) { return bin.x<x; });
        return value_impl(x,it);
    }

    /**
     * @brief Integrate the distribution between x0 and x1
     * @param x0 integrate from
     * @param x1 integrate to
     */
    [[nodiscard]] inline f_t integrate(f_t x0, f_t x1) const noexcept {
        auto it = std::lower_bound(bins.begin(), bins.end(), x0, 
                                   [](const auto& bin, const auto x) { return bin.x<x; });
        if (it==bins.begin()) {
            if (x1<=it->x) return 0;
            x0 = m::max(bins[0].x,x0);
        }
        if (it==bins.end()) return 0;

        f_t val = 0;
        auto v0 = vec2_t{ x0,value_impl(x0,it) };
        do {
            const auto v1 = it->x<=x1 ? 
                            *it : vec2_t{ x1,value_impl(x1,it) };
            val += (v1.x-v0.x)*(v1.y+v0.y)/2;

            v0 = v1;
            ++it;
        } while (it!=bins.end() && std::prev(it)->x<x1);

        return val;
    }

    /**
     * @brief PDF
     * @param x value
     * @return PDF of the distribution at x
     */
    [[nodiscard]] inline f_t pdf(f_t x, 
                                 measure_e measure = measure_e::continuos) const noexcept final {
        return measure==measure_e::continuos ? value(x)*norm : 0;
    }

    /**
     * @brief Inverse CDF
     * @param v CDF values in range [0,1]
     * @return value
     */
    [[nodiscard]] inline vec2_t icdf(f_t v) const noexcept {
        assert(v>=0 && v<=1);

        const auto it = std::ranges::lower_bound(dcdf, v);
        auto idx = m::clamp<std::ptrdiff_t>(std::ptrdiff_t(it-dcdf.begin())-1, 0, dcdf.size()-2);
        for (; idx+1<dcdf.size() && dcdf[idx+1]-dcdf[idx]==0; ++idx) {}

        if (idx+1==dcdf.size())
            return bins[idx];

        assert(v>=dcdf[idx] && v<=dcdf[idx+1]);

        const auto f = (v-dcdf[idx])/(dcdf[idx+1]-dcdf[idx]);
        const auto& a = bins[idx];
        const auto& b = bins[idx+1];

        if (a.y==b.y)
            return m::mix(a,b,f);

        const auto m = m::mix(m::sqr(a.y), m::sqr(b.y), f);
        const auto d = m::sqrt(m);
        const auto t = m::clamp01((a.y-d)/(a.y-b.y));

        assert(m::isfinite(t));
        assert(m>=0);

        return m::mix(bins[idx], bins[idx+1], t);
    }
    /**
     * @brief Sample from the distribution
     */
    [[nodiscard]] sample_ret_t sample(sampler::sampler_t& sampler) const noexcept final {
        const auto& val = icdf(sampler.r());
        return {
            .x = val.x,
            .measure = measure_e::continuos,
            .pdf = val.y*norm,
        };
    }
};

}
