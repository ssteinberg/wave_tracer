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
#include "piecewise_linear_distribution.hpp"

namespace wt {

/**
 * @brief Continuos piecewise-linear distribution using equal-spaced bins for fast lookup performance.
 *        Also bins the iCDF for fast sampling.
 */
class binned_piecewise_linear_distribution_t final : public distribution1d_t {
private:
    std::vector<f_t> ys;
    std::vector<f_t> dcdf;
    std::vector<std::uint32_t> binned_icdf;
    range_t<> xrange;
    f_t dx, recp_dx;
    f_t sum, norm;

public:
    /**
     * @brief Construct a binned_piecewise_linear_distribution_t
     * @param values non negative values, with x in strictly ascending order
     */
    explicit binned_piecewise_linear_distribution_t(std::vector<f_t> yvalues, const range_t<> xrange)
        : ys(std::move(yvalues)),
          xrange(xrange)
    {
        assert(ys.size()>1);
        if (ys.size()<=1)
            return;

        dx = xrange.length() / (ys.size()-1);
        recp_dx = 1/dx;
        assert(dx>0);

        // accumulate
        dcdf.resize(ys.size());
        dcdf[0]=0;
        for (std::size_t i=1;i<ys.size();++i)
            dcdf[i] = dcdf[i-1] + dx*(ys[i]+ys[i-1])/2;
        
        // normalize
        sum = dcdf.back();
        const auto recp_sum = sum>0 ? f_t(1)/sum : f_t(0);
        std::ranges::transform(dcdf, dcdf.begin(), 
                               [n=recp_sum](const auto c) { return c*n; });
        norm = recp_sum;

        // build binned iCDF
        assert(limits<std::uint32_t>::max()>=dcdf.size());

        // TODO: how large should this be?
        binned_icdf.resize(4*ys.size());
        const auto rcp_bicdf = double(1) / binned_icdf.size();

        std::uint32_t idx=0;
        for (std::size_t i=0;i<binned_icdf.size();++i) {
            const auto x = i * rcp_bicdf;
            while (idx+1<dcdf.size() && (dcdf[idx+1]<x || dcdf[idx+1]==dcdf[idx])) ++idx;
            binned_icdf[i] = idx;
        }
    }

    /**
     * @brief Construct a binned_piecewise_linear_distribution_t
     * @param pwld a piecewise-linear distribution
     * @param min_dx (optional) minimal bin size
     */
    explicit binned_piecewise_linear_distribution_t(const piecewise_linear_distribution_t& pwld,
                                                    const range_t<> range = range_t<>::all(),
                                                    const std::optional<f_t> min_dx = {}) {
        if (pwld.size()<2)
            return;

        // dx by default is the dx mean - stddev
        // this chooses a conservative dx while being resilient to user input
        double dx_sum = 0, dx2_sum = 0;
        std::size_t samples = 0;
        for (int i=0;i<pwld.size()-1;++i) {
            if (pwld[i+1].x>pwld[i].x) {
                dx_sum += pwld[i+1].x-pwld[i].x;
                dx2_sum += m::sqr(double(pwld[i+1].x-pwld[i].x));
                ++samples;
            }
        }
        assert(samples>1);
        const auto dx_stddev = m::sqrt(
                    (dx2_sum*samples - m::sqr(dx_sum)) / 
                    (samples*(samples-1))
                );
        dx = m::clamp<f_t>(dx_sum/samples - dx_stddev, min_dx.value_or(0), range.length());
        if (dx==0 || !m::isfinite(dx) || samples<=1) {
            throw std::runtime_error("cannot create binned spectrum, input may have too large variation in step size.");
            return;
        }

        const auto xrange = pwld.range() & range;
        const auto bins = m::max(1ul, std::size_t(xrange.length() / dx)) + 1;

        std::vector<f_t> ys;
        ys.resize(bins);
        const auto rcp_bins = f_t(1) / (bins-1);
        for (int i=0;i<bins;++i)
            ys[i] = pwld.value(m::min(xrange.max, m::mix(xrange,i*rcp_bins)));

        *this = binned_piecewise_linear_distribution_t{ std::move(ys), xrange };
    }


    binned_piecewise_linear_distribution_t(binned_piecewise_linear_distribution_t&&) noexcept = default;
    binned_piecewise_linear_distribution_t(const binned_piecewise_linear_distribution_t&) noexcept = default;
    binned_piecewise_linear_distribution_t& operator=(binned_piecewise_linear_distribution_t&&) noexcept = default;
    binned_piecewise_linear_distribution_t& operator=(const binned_piecewise_linear_distribution_t&) noexcept = default;

    [[nodiscard]] std::unique_ptr<distribution1d_t> clone() const override {
        return std::make_unique<binned_piecewise_linear_distribution_t>(*this);
    }

    [[nodiscard]] inline auto xstep() const noexcept { return dx; }
    [[nodiscard]] inline auto total() const noexcept { return sum; }
    [[nodiscard]] inline auto range() const noexcept { return xrange; }

    [[nodiscard]] inline auto& operator[](std::size_t idx) { return ys[idx]; }
    [[nodiscard]] inline const auto& operator[](std::size_t idx) const { return ys[idx]; }

    [[nodiscard]] inline auto size() const noexcept { return ys.size(); }

    [[nodiscard]] inline auto begin() noexcept { return ys.begin(); }
    [[nodiscard]] inline auto end() noexcept { return ys.end(); }
    [[nodiscard]] inline auto begin() const noexcept { return ys.begin(); }
    [[nodiscard]] inline auto end() const noexcept { return ys.end(); }

    [[nodiscard]] inline auto& front() noexcept { return ys.front(); }
    [[nodiscard]] inline auto& back() noexcept { return ys.back(); }
    [[nodiscard]] inline const auto& front() const noexcept { return ys.front(); }
    [[nodiscard]] inline const auto& back() const noexcept { return ys.back(); }

    [[nodiscard]] inline auto cbegin() const noexcept { return ys.cbegin(); }
    [[nodiscard]] inline auto cend() const noexcept { return ys.cend(); }
    [[nodiscard]] inline auto crbegin() const noexcept { return ys.crbegin(); }
    [[nodiscard]] inline auto crend() const noexcept { return ys.crend(); }

    [[nodiscard]] inline auto rbegin() noexcept { return ys.rbegin(); }
    [[nodiscard]] inline auto rend() noexcept { return ys.rend(); }
    [[nodiscard]] inline auto rbegin() const noexcept { return ys.rbegin(); }
    [[nodiscard]] inline auto rend() const noexcept { return ys.rend(); }

    [[nodiscard]] inline auto operator+(const binned_piecewise_linear_distribution_t& d) const noexcept {
        // only supports distributions with identical bin count and range
        if (d.range() != this->range() || d.ys.size() != this->ys.size()) {
            assert(false);
            return binned_piecewise_linear_distribution_t{ {}, {} };
        }

        auto ys = this->ys;
        for (std::size_t i=0;i<ys.size();++i)
            ys[i] += d.ys[i];

        return binned_piecewise_linear_distribution_t{ std::move(ys), this->range() };
    }

    /**
     * @brief Value at position
     * @param x position
     * @return Piece-wise linearly interpolated value
     */
    [[nodiscard]] inline f_t value(f_t x) const noexcept {
        const auto bin = (x-xrange.min) * recp_dx;
        if (bin<0 || bin>ys.size()-1) return 0;

        const auto i = (std::size_t)bin;
        const auto f = m::fract(bin);

        return m::mix(ys[i], ys[m::min(ys.size()-1,i+1)], f);
    }

    /**
     * @brief Integrate the distribution between x0 and x1
     * @param x0 integrate from
     * @param x1 integrate to
     */
    [[nodiscard]] inline f_t integrate(f_t x0, f_t x1) const noexcept {
        if (x0<=xrange.min && x1>=xrange.max)
            return sum;

        const auto bin0 = m::max<f_t>(0, (x0-xrange.min) * recp_dx);
        const auto bin1 = m::max<f_t>(0,(x1-xrange.min) * recp_dx);
        const auto i0 = (std::size_t)bin0;
        const auto f0 = m::isfinite(bin0) ? m::fract(bin0) : 0;
        const auto i1 = (std::size_t)bin1;
        const auto f1 = m::isfinite(bin1) ? m::fract(bin1) : 0;

        f_t val=0;
        for (int i=m::max<int>(0,i0+1); i<m::min<int>(i1,ys.size()-1); ++i)
            val += dx*(ys[i+1]+ys[i])/2;

        if (i1>i0 && i0+1<ys.size()) {
            const auto y1 = ys[i0+1];
            const auto y0 = m::mix(ys[i0], y1, f0);
            val += (1-f0)*dx*(y0+y1)/2;
        }
        if (i1>i0 && i1+1<ys.size()) {
            const auto y0 = ys[i1];
            const auto y1 = m::mix(y0, ys[i1+1], f1);
            val += f1*dx*(y0+y1)/2;
        }
        if (i1==i0 && i0+1<ys.size()) {
            const auto ya = ys[i0];
            const auto yb = ys[i0+1];
            const auto y0 = m::mix(ya, yb, f0);
            const auto y1 = m::mix(ya, yb, f1);
            val += dx*m::max<f_t>(0,f1-f0)*(y0+y1)/2;
        }

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
     * @brief Inverse CDF, binned version
     * @param v CDF values in range [0,1]
     * @return value
     */
    [[nodiscard]] inline vec2_t icdf(f_t v) const noexcept {
        assert(v>=0 && v<=1);

        const auto bin = v * binned_icdf.size();
        const auto i = (std::size_t)bin;
        auto idx = i>=0 && i<binned_icdf.size() ? binned_icdf[i] : dcdf.size()-1;

        while (idx>0 && v<dcdf[idx]) --idx;
        while (idx+1<dcdf.size()-1 && v>dcdf[idx+1]) ++idx;
        if (idx+1>=dcdf.size())
            return { xrange.min + idx*dx, ys[idx] };

        assert(v>=dcdf[idx] && v<=dcdf[idx+1]);

        const auto f = (v-dcdf[idx])/(dcdf[idx+1]-dcdf[idx]);
        const auto& a = ys[idx];
        const auto& b = ys[idx+1];

        if (a==b)
            return vec2_t{ (idx+f)*dx,a };

        const auto m = m::mix(m::sqr(a), m::sqr(b), f);
        const auto d = m::sqrt(m);
        const auto t = m::clamp01((a-d)/(a-b));

        assert(m::isfinite(t));
        assert(m>=0);

        return m::mix(vec2_t{ xrange.min + idx*dx,a },vec2_t{ xrange.min + (idx+1)*dx,b },t);
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
