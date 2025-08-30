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
#include <numeric>
#include <algorithm>

#include <cassert>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>

#include "distribution1d.hpp"

namespace wt {

/**
 * @brief Discrete distribution (sum of Dirac impulses)
 */
template <typename T>
class discrete_distribution_t {
private:
    std::vector<T> bins;
    std::vector<f_t> dcdf;

public:
    /**
     * @brief Construct a discrete_distribution_t
     * @param values values
     * @param densities probability density for each value
     */
    explicit discrete_distribution_t(std::vector<T> values, const std::vector<f_t>& densities) 
        : bins(std::move(values))
    {
        assert(bins.size() && bins.size()==densities.size());

        // accumulate
        dcdf.resize(densities.size()+1);
        dcdf[0]=0;
        std::partial_sum(densities.cbegin(), densities.cend(), std::next(dcdf.begin()));
        
        // normalize
        const auto sum = dcdf.back();
        if (sum>0) {
            const auto recp_sum = f_t(1)/sum;
            std::transform(dcdf.cbegin(), dcdf.cend(), dcdf.begin(), 
                           [=](const auto c) { return c*recp_sum; });
        } else {
            dcdf.back() = 1;
        }
    }

    /**
     * @brief Construct a discrete_distribution_t
     * @param values values
     * @param density value to density functor
     */
    template <typename Func>
    explicit discrete_distribution_t(std::vector<T> values, Func density) 
        : bins(std::move(values))
    {
        assert(bins.size());

        // accumulate
        dcdf.resize(bins.size()+1);
        dcdf[0]=0;
        for (std::size_t i=0;i<bins.size();++i) {
            const auto pd = density(bins[i]);
            assert(pd>=0);
            dcdf[i+1] = dcdf[i] + m::max<f_t>(0,pd);
        }
        
        // normalize
        const auto sum = dcdf.back();
        if (sum>0) {
            const auto recp_sum = f_t(1)/sum;
            std::transform(dcdf.cbegin(), dcdf.cend(), dcdf.begin(), 
                           [=](const auto c) { return c*recp_sum; });
        } else {
            dcdf.back() = 1;
        }
    }
    explicit discrete_distribution_t(std::vector<T> values) requires std::is_same_v<T,f_t>
        : discrete_distribution_t(std::move(values), [](const auto t) { return t; })
    {}

    discrete_distribution_t(discrete_distribution_t&&) noexcept = default;
    discrete_distribution_t(const discrete_distribution_t&) noexcept = default;
    discrete_distribution_t& operator=(discrete_distribution_t&&) noexcept = default;
    discrete_distribution_t& operator=(const discrete_distribution_t&) noexcept = default;

    [[nodiscard]] inline auto& operator[](std::size_t idx) { return bins[idx]; }
    [[nodiscard]] inline const auto& operator[](std::size_t idx) const { return bins[idx]; }

    [[nodiscard]] inline auto size() const noexcept { return bins.size(); }

    [[nodiscard]] inline auto begin() noexcept { return bins.begin(); }
    [[nodiscard]] inline auto end() noexcept { return bins.end(); }
    [[nodiscard]] inline auto begin() const noexcept { return bins.begin(); }
    [[nodiscard]] inline auto end() const noexcept { return bins.end(); }

    [[nodiscard]] inline auto front() noexcept { return bins.front(); }
    [[nodiscard]] inline auto back() noexcept { return bins.back(); }
    [[nodiscard]] inline auto front() const noexcept { return bins.front(); }
    [[nodiscard]] inline auto back() const noexcept { return bins.back(); }

    [[nodiscard]] inline auto cbegin() const noexcept { return bins.cbegin(); }
    [[nodiscard]] inline auto cend() const noexcept { return bins.cend(); }
    [[nodiscard]] inline auto crbegin() const noexcept { return bins.crbegin(); }
    [[nodiscard]] inline auto crend() const noexcept { return bins.crend(); }

    [[nodiscard]] inline auto rbegin() noexcept { return bins.rbegin(); }
    [[nodiscard]] inline auto rend() noexcept { return bins.rend(); }
    [[nodiscard]] inline auto rbegin() const noexcept { return bins.rbegin(); }
    [[nodiscard]] inline auto rend() const noexcept { return bins.rend(); }

    /**
     * @brief PDF
     * @param idx bin index
     * @return PDF of the bin
     */
    [[nodiscard]] inline auto pdf(std::size_t idx) const noexcept { return dcdf[idx+1]-dcdf[idx]; }

    /**
     * @brief CDF
     * @param idx bin index
     * @return CDF of the bin
     */
    [[nodiscard]] inline auto cdf(std::size_t idx) const noexcept { return dcdf[idx+1]; }

    /**
     * @brief Inverse CDF
     * @param v CDF values in range [0,1]
     * @return index of bin
     */
    [[nodiscard]] inline auto icdf(f_t v) const noexcept {
        const auto it = std::ranges::lower_bound(dcdf, v);
        auto idx = m::clamp<std::ptrdiff_t>(std::ptrdiff_t(it-dcdf.begin())-1, 0, bins.size()-1);
        for (; idx<bins.size()-1 && dcdf[idx+1]-dcdf[idx]==0; ++idx) {}

        return idx;
    }


    [[nodiscard]] std::vector<f_t> tabulate(const range_t<>& range, std::size_t bc) const {
        std::vector<f_t> tbl;
        tbl.resize(bc, 0);

        for (auto idx=0ul; idx<this->bins.size(); ++idx) {
            const auto& b = this->bins[idx];
            if (range.contains(b)) {
                const auto idx = (b-range.min)/range.length() * bc;
                tbl[m::clamp<int>((int)idx,0,tbl.size()-1)] += this->pdf(idx);
            }
        }

        return tbl;
    }
};

template <>
class discrete_distribution_t<vec2_t> : public distribution1d_t {
private:
    std::vector<vec2_t> bins;
    std::vector<f_t> dcdf;
    f_t sum, recp_sum;

public:
    /**
     * @brief Construct a discrete_distribution_t
     * @param values values
     */
    explicit discrete_distribution_t(std::vector<vec2_t> values) 
        : bins(std::move(values))
    {
        assert(bins.size());

        // accumulate
        dcdf.resize(bins.size()+1);
        dcdf[0]=0;
        for (std::size_t i=0;i<bins.size();++i) {
            const auto pd = bins[i].y;
            assert(pd>=0);
            dcdf[i+1] = dcdf[i] + m::max<f_t>(0,pd);
        }
        
        // normalize
        sum = dcdf.back();
        recp_sum = sum>0 ? f_t(1)/sum : f_t(0);
        std::transform(dcdf.cbegin(), dcdf.cend(), dcdf.begin(), 
                       [n=this->recp_sum](const auto c) { return c*n; });
        if (sum==0) dcdf.back() = 1;
    }

    discrete_distribution_t(discrete_distribution_t&&) noexcept = default;
    discrete_distribution_t(const discrete_distribution_t&) noexcept = default;
    discrete_distribution_t& operator=(discrete_distribution_t&&) noexcept = default;
    discrete_distribution_t& operator=(const discrete_distribution_t&) noexcept = default;

    [[nodiscard]] std::unique_ptr<distribution1d_t> clone() const override {
        return std::make_unique<discrete_distribution_t<vec2_t>>(*this);
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

    /**
     * @brief PDF
     * @param x value
     * @return PDF of the distribution at x
     */
    [[nodiscard]] inline f_t pdf(f_t x, 
                                 measure_e measure) const noexcept final {
        const auto it = std::lower_bound(cbegin(), cend(), x, 
                                         [](const auto& b, auto v) { return b.x<v; });
        if (it==cend() || it->x!=x) return 0;
        const auto idx = std::distance(cbegin(), it);
        return dcdf[idx+1]-dcdf[idx];
    }

    /**
     * @brief CDF
     */
    [[nodiscard]] inline f_t cdf(f_t x) const noexcept {
        const auto it = std::lower_bound(cbegin(), cend(), x, 
                                         [](const auto& b, auto v) { return b.x<v; });
        if (it==cend() || it->x!=x) return 0;
        const auto idx = std::distance(cbegin(), it);
        return dcdf[idx+1];
    }

    /**
     * @brief Inverse CDF
     */
    [[nodiscard]] inline vec2_t icdf(f_t v) const noexcept {
        const auto it = std::ranges::lower_bound(dcdf, v);
        auto idx = m::clamp<std::ptrdiff_t>(std::ptrdiff_t(it-dcdf.begin())-1, 0, bins.size()-1);
        for (; idx<bins.size()-1 && dcdf[idx+1]-dcdf[idx]==0; ++idx) {}

        return bins[idx];
    }
    /**
     * @brief Sample from the distribution
     * @return sampled x,y value
     */
    [[nodiscard]] sample_ret_t sample(sampler::sampler_t& sampler) const noexcept final {
        const auto& val = icdf(sampler.r());
        return {
            .x = val.x,
            .measure = measure_e::discrete,
            .pdf = val.y*recp_sum,
        };
    }


    [[nodiscard]] std::vector<f_t> tabulate(const range_t<>& range, std::size_t bc) const override {
        std::vector<f_t> tbl;
        tbl.resize(bc, 0);

        for (auto idx=0ul; idx<this->bins.size(); ++idx) {
            const auto& b = this->bins[idx];
            if (range.contains(b.x)) {
                const auto idx = (b.x-range.min)/range.length() * bc;
                tbl[m::clamp<int>((int)idx,0,tbl.size()-1)] += b.y;
            }
        }

        return tbl;
    }
};

}
