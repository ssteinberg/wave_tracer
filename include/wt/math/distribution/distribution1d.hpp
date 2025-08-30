/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <vector>
#include <cassert>

#include <wt/sampler/sampler.hpp>
#include <wt/sampler/measure.hpp>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>

namespace wt {

class distribution1d_t {
public:
    virtual ~distribution1d_t() noexcept = default;
    
    [[nodiscard]] virtual std::unique_ptr<distribution1d_t> clone() const = 0;

    /**
     * @brief PDF
     * @param x value
     * @return PDF of the distribution at x
     */
    [[nodiscard]] virtual f_t pdf(f_t x, 
                                  measure_e measure = measure_e::continuos) const noexcept = 0;

    struct sample_ret_t {
        /** @brief Sampled point. */
        f_t x;
        /** @brief Measure of sampled point. `wt::measure */
        measure_e measure;
        f_t pdf;
    };

    /**
     * @brief Sample from the distribution
     */
    [[nodiscard]] virtual sample_ret_t sample(sampler::sampler_t& sampler) const noexcept = 0;


    [[nodiscard]] virtual std::vector<f_t> tabulate(const range_t<>& range, std::size_t bins) const {
        std::vector<f_t> tbl;
        tbl.resize(bins);
        const auto rcp_bc = f_t(1)/bins;
        for (auto i=0ul;i<bins;++i)
            tbl[i] = pdf(m::mix(range, (i+f_t(.5))*rcp_bc));

        return tbl;
    }
};

}
