/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <vector>
#include <ranges>
#include <algorithm>
#include <iterator>

#include <wt/math/distribution/product_distribution.hpp>
#include <wt/math/common.hpp>

namespace wt {


[[nodiscard]] inline auto make_pwl_product_dist(std::vector<vec2_t> bins) noexcept {
    if (bins.size()<=1) {
        assert(false);
        bins = { vec2_t{0,0}, vec2_t{1,0} };
    }

    auto dist = piecewise_linear_distribution_t{ std::move(bins) };
    const auto R0 = dist.total();
    assert(R0>=0 && R0<=1);

    return product_distribution_ret_t<piecewise_linear_distribution_t>{
        .dist = std::move(dist),
        .R0 = R0,
        .approximate = true,
    };
}


product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& f,
                         const discrete_distribution_t<vec2_t>& g) {
    std::vector<vec2_t> bins;
    bins.reserve(m::max(f.size(), g.size()));

    f_t R0 = 0;
    for (auto it1 = f.cbegin(); it1 != f.cend(); ++it1)
    for (auto it2 = g.cbegin(); it2 != g.cend(); ++it2) {
        if (it1->x==it2->x) {
            const auto pdf1 = f.pdf(it1->x, measure_e::discrete);
            const auto pdf2 = g.pdf(it2->x, measure_e::discrete);

            R0 += pdf1*pdf2;
            bins.emplace_back(it1->x,pdf1*pdf2);
        }
    }

    if (bins.empty()) bins.emplace_back(0,0);

    return {
        .dist = discrete_distribution_t<vec2_t>{ std::move(bins) },
        .R0 = R0,
        .approximate = false,
        .discrete = true,
    };
}

product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& pm,
                         const uniform_distribution_t& dist) {
    std::vector<vec2_t> bins;
    bins.reserve(pm.size());

    f_t R0 = 0;
    for (auto it1 = pm.cbegin(); it1 != pm.cend(); ++it1) {
        if (!m::isfinite(it1->x)) continue;

        const auto pdf1 = pm.pdf(it1->x, measure_e::discrete);
        const auto pdf2 = dist.pdf(it1->x);

        R0 += pdf1*pdf2;
        bins.emplace_back(it1->x, pdf1*pdf2);
    }

    if (bins.empty()) bins.emplace_back(0,0);

    return {
        .dist = discrete_distribution_t<vec2_t>{ std::move(bins) },
        .R0 = R0,
        .approximate = false,
    };
}

product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& pm,
                         const piecewise_linear_distribution_t& dist) {
    std::vector<vec2_t> bins;
    bins.reserve(pm.size());

    f_t R0 = 0;
    for (auto it1 = pm.cbegin(); it1 != pm.cend(); ++it1) {
        const auto pdf1 = pm.pdf(it1->x, measure_e::discrete);
        const auto pdf2 = dist.pdf(it1->x);

        R0 += pdf1*pdf2;
        bins.emplace_back(it1->x, pdf1*pdf2);
    }

    if (bins.empty()) bins.emplace_back(0,0);

    return {
        .dist = discrete_distribution_t<vec2_t>{ std::move(bins) },
        .R0 = R0,
        .approximate = false,
    };
}

product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& pm,
                         const binned_piecewise_linear_distribution_t& dist) {
    std::vector<vec2_t> bins;
    bins.reserve(pm.size());

    f_t R0 = 0;
    for (auto it1 = pm.cbegin(); it1 != pm.cend(); ++it1) {
        const auto pdf1 = pm.pdf(it1->x, measure_e::discrete);
        const auto pdf2 = dist.pdf(it1->x);

        R0 += pdf1*pdf2;
        bins.emplace_back(it1->x, pdf1*pdf2);
    }

    if (bins.empty()) bins.emplace_back(0,0);

    return {
        .dist = discrete_distribution_t<vec2_t>{ std::move(bins) },
        .R0 = R0,
        .approximate = false,
    };
}

product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& pm,
                         const gaussian1d_t& dist) {
    std::vector<vec2_t> bins;
    bins.reserve(pm.size());

    f_t R0 = 0;
    for (auto it1 = pm.cbegin(); it1 != pm.cend(); ++it1) {
        const auto pdf1 = pm.pdf(it1->x, measure_e::discrete);
        const auto pdf2 = dist.pdf(it1->x);

        R0 += pdf1*pdf2;
        if (pdf1*pdf2>0)
        bins.emplace_back(it1->x, pdf1*pdf2);
    }

    if (bins.empty()) bins.emplace_back(0,0);

    return {
        .dist = discrete_distribution_t<vec2_t>{ std::move(bins) },
        .R0 = R0,
        .approximate = false,
    };
}

product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& pm,
                         const truncated_gaussian1d_t& dist) {
    std::vector<vec2_t> bins;
    bins.reserve(pm.size());

    f_t R0 = 0;
    for (auto it1 = pm.cbegin(); it1 != pm.cend(); ++it1) {
        const auto pdf1 = pm.pdf(it1->x, measure_e::discrete);
        const auto pdf2 = dist.pdf(it1->x);

        R0 += pdf1*pdf2;
        if (pdf1*pdf2>0)
        bins.emplace_back(it1->x, pdf1*pdf2);
    }

    if (bins.empty()) bins.emplace_back(0,0);

    return {
        .dist = discrete_distribution_t<vec2_t>{ std::move(bins) },
        .R0 = R0,
        .approximate = false,
    };
}


// Takes 2 sorted views or containers and merges them into a sorted std::vector<f_t>.
inline auto sorted_candidates(const auto& v1, const auto& v2) noexcept {
    std::vector<f_t> vs;
    vs.reserve(v1.size()+v2.size());
    std::ranges::merge(v1,v2, std::back_inserter(vs));
    return vs;
}

product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const piecewise_linear_distribution_t& f,
                         const piecewise_linear_distribution_t& g) {
    const auto xs = sorted_candidates(
        f | std::views::transform([](const auto& v) { return v.x; }),
        g | std::views::transform([](const auto& v) { return v.x; }));

    // TODO: this can be done accurately with a piecewise quadratic
    constexpr auto pts = 2ul;

    std::vector<vec2_t> bins;
    bins.reserve(pts*(xs.size()-1)+1);
    for (auto it=xs.cbegin(); it!=xs.cend(); ++it) {
        const auto p = f.pdf(*it)*g.pdf(*it);
        bins.emplace_back(*it, p);

        if (std::next(it)!=xs.cend()) {
            for (std::size_t i=1;i<pts;++i) {
                const auto x = m::mix(*it,*std::next(it),i/f_t(pts));
                const auto p = f.pdf(x)*g.pdf(x);
                bins.emplace_back(x, p);
            }
        }
    }

    return make_pwl_product_dist(std::move(bins));
}

product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const piecewise_linear_distribution_t& dist1,
                         const uniform_distribution_t& dist2) {
    const auto xs = sorted_candidates(
        dist1 | std::views::transform([](const auto& v) { return v.x; }),
        dist2.range());

    std::vector<vec2_t> bins;
    bins.reserve(xs.size());
    for (const auto x : xs) {
        if (!m::isfinite(x)) continue;

        const auto p = dist1.pdf(x)*dist2.pdf(x);
        bins.emplace_back(x, p);
    }

    return make_pwl_product_dist(std::move(bins));
}

product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const piecewise_linear_distribution_t& dist1,
                         const gaussian1d_t& dist2) {
    constexpr auto pts_per_stddev = 6;
    constexpr auto stddevs = 5;
    std::vector<f_t> gxs;
    gxs.reserve((2*stddevs+1) * pts_per_stddev);
    for (int s=-stddevs; s<stddevs; ++s)
    for (int p=0; p<pts_per_stddev + (s==stddevs-1?1:0); ++p) {
        const auto x = dist2.mean() + dist2.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        gxs.emplace_back(x);
    }

    const auto xs = sorted_candidates(
        dist1 | std::views::transform([](const auto& v) { return v.x; }),
        gxs);

    std::vector<vec2_t> bins;
    bins.reserve(xs.size());
    for (const auto x : xs) {
        const auto p = dist1.pdf(x)*dist2.pdf(x);
        bins.emplace_back(x, p);
    }

    return make_pwl_product_dist(std::move(bins));
}

product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const piecewise_linear_distribution_t& dist1,
                         const truncated_gaussian1d_t& dist2) {
    constexpr auto pts_per_stddev = 6;
    constexpr auto stddevs = 5;
    std::vector<f_t> gxs;
    gxs.reserve((2*stddevs+1) * pts_per_stddev);
    for (int s=-stddevs; s<stddevs; ++s)
    for (int p=0; p<pts_per_stddev + (s==stddevs-1?1:0); ++p) {
        const auto x = dist2.mean() + dist2.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        gxs.emplace_back(x);
    }

    const auto xs = sorted_candidates(
        dist1 | std::views::transform([](const auto& v) { return v.x; }),
        gxs);

    std::vector<vec2_t> bins;
    bins.reserve(xs.size());
    for (const auto x : xs) {
        const auto p = dist1.pdf(x)*dist2.pdf(x);
        bins.emplace_back(x, p);
    }

    return make_pwl_product_dist(std::move(bins));
}


product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const binned_piecewise_linear_distribution_t& dist1,
                         const binned_piecewise_linear_distribution_t& dist2) {
    std::vector<f_t> xs1, xs2;
    xs1.reserve(dist1.size());
    xs2.reserve(dist2.size());
    const auto recp_size1 = 1/f_t(dist1.size()-1);
    const auto recp_size2 = 1/f_t(dist2.size()-1);
    for (auto i=0ul; i<dist1.size(); ++i)
        xs1.emplace_back(m::mix(dist1.range(), f_t(i)*recp_size1));
    for (auto i=0ul; i<dist2.size(); ++i)
        xs2.emplace_back(m::mix(dist2.range(), f_t(i)*recp_size2));

    const auto xs = sorted_candidates(xs1, xs2);

    // TODO: this can be done accurately with a piecewise quadratic
    constexpr auto pts = 2ul;

    std::vector<vec2_t> bins;
    bins.reserve(pts*(xs.size()-1)+1);
    for (auto it=xs.cbegin(); it!=xs.cend(); ++it) {
        const auto p = dist1.pdf(*it)*dist2.pdf(*it);
        bins.emplace_back(*it, p);

        if (std::next(it)!=xs.cend()) {
            for (std::size_t i=1;i<pts;++i) {
                const auto x = m::mix(*it,*std::next(it),i/f_t(pts));
                const auto p = dist1.pdf(x)*dist2.pdf(x);
                bins.emplace_back(x, p);
            }
        }
    }

    return make_pwl_product_dist(std::move(bins));
}

product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const binned_piecewise_linear_distribution_t& dist1,
                         const uniform_distribution_t& dist2) {
    std::vector<f_t> xs1;
    xs1.reserve(dist1.size());
    const auto recp_size = 1/f_t(dist1.size()-1);
    for (auto i=0ul; i<dist1.size(); ++i)
        xs1.emplace_back(m::mix(dist1.range(), f_t(i)*recp_size));

    const auto xs = sorted_candidates(
        xs1,
        dist2.range());

    std::vector<vec2_t> bins;
    bins.reserve(xs.size());
    for (const auto x : xs) {
        if (!m::isfinite(x)) continue;

        const auto p = dist1.pdf(x)*dist2.pdf(x);
        bins.emplace_back(x, p);
    }

    return make_pwl_product_dist(std::move(bins));
}

product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const binned_piecewise_linear_distribution_t& dist1,
                         const gaussian1d_t& dist2) {
    std::vector<f_t> xs1;
    xs1.reserve(dist1.size());
    const auto recp_size = 1/f_t(dist1.size()-1);
    for (auto i=0ul; i<dist1.size(); ++i)
        xs1.emplace_back(m::mix(dist1.range(), f_t(i)*recp_size));

    constexpr auto pts_per_stddev = 6;
    constexpr auto stddevs = 5;
    std::vector<f_t> gxs;
    gxs.reserve((2*stddevs+1) * pts_per_stddev);
    for (int s=-stddevs; s<stddevs; ++s)
    for (int p=0; p<pts_per_stddev + (s==stddevs-1?1:0); ++p) {
        const auto x = dist2.mean() + dist2.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        gxs.emplace_back(x);
    }

    const auto xs = sorted_candidates(xs1, gxs);

    std::vector<vec2_t> bins;
    bins.reserve(xs.size());
    for (const auto x : xs) {
        const auto p = dist1.pdf(x)*dist2.pdf(x);
        bins.emplace_back(x, p);
    }

    return make_pwl_product_dist(std::move(bins));
}

product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const binned_piecewise_linear_distribution_t& dist1,
                         const truncated_gaussian1d_t& dist2) {
    std::vector<f_t> xs1;
    xs1.reserve(dist1.size());
    const auto recp_size = 1/f_t(dist1.size()-1);
    for (auto i=0ul; i<dist1.size(); ++i)
        xs1.emplace_back(m::mix(dist1.range(), f_t(i)*recp_size));

    constexpr auto pts_per_stddev = 6;
    constexpr auto stddevs = 5;
    std::vector<f_t> gxs;
    gxs.reserve((2*stddevs+1) * pts_per_stddev);
    for (int s=-stddevs; s<stddevs; ++s)
    for (int p=0; p<pts_per_stddev + (s==stddevs-1?1:0); ++p) {
        const auto x = dist2.mean() + dist2.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        gxs.emplace_back(x);
    }

    const auto xs = sorted_candidates(xs1, gxs);

    std::vector<vec2_t> bins;
    bins.reserve(xs.size());
    for (const auto x : xs) {
        const auto p = dist1.pdf(x)*dist2.pdf(x);
        bins.emplace_back(x, p);
    }

    return make_pwl_product_dist(std::move(bins));
}


product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const piecewise_linear_distribution_t& dist1,
                         const binned_piecewise_linear_distribution_t& dist2) {
    std::vector<f_t> xs1, xs2;
    xs1.reserve(dist1.size());
    xs2.reserve(dist2.size());
    const auto recp_size = 1/f_t(dist2.size()-1);
    for (const auto& v : dist1)
        xs1.emplace_back(v.x);
    for (auto i=0ul; i<dist2.size(); ++i)
        xs2.emplace_back(m::mix(dist2.range(), f_t(i)/recp_size));

    const auto xs = sorted_candidates(xs1, xs2);

    // TODO: this can be done accurately with a piecewise quadratic
    constexpr auto pts = 2ul;

    std::vector<vec2_t> bins;
    bins.reserve(pts*(xs.size()-1)+1);
    for (auto it=xs.cbegin(); it!=xs.cend(); ++it) {
        const auto p = dist1.pdf(*it)*dist2.pdf(*it);
        bins.emplace_back(*it, p);

        if (std::next(it)!=xs.cend()) {
            for (std::size_t i=1;i<pts;++i) {
                const auto x = m::mix(*it,*std::next(it),i/f_t(pts));
                const auto p = dist1.pdf(x)*dist2.pdf(x);
                bins.emplace_back(x, p);
            }
        }
    }

    return make_pwl_product_dist(std::move(bins));
}


product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const gaussian1d_t& dist1,
                         const gaussian1d_t& dist2) {
    constexpr auto pts_per_stddev = 6;
    constexpr auto stddevs = 5;
    std::vector<f_t> gxs1, gxs2;
    gxs1.reserve((2*stddevs+1) * pts_per_stddev);
    gxs2.reserve((2*stddevs+1) * pts_per_stddev);
    for (int s=-stddevs; s<stddevs; ++s)
    for (int p=0; p<pts_per_stddev + (s==stddevs-1?1:0); ++p) {
        const auto x1 = dist1.mean() + dist1.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        const auto x2 = dist2.mean() + dist2.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        gxs1.emplace_back(x1);
        gxs2.emplace_back(x2);
    }

    const auto xs = sorted_candidates(gxs1, gxs2);

    std::vector<vec2_t> bins;
    bins.reserve(xs.size());
    for (const auto x : xs) {
        const auto p = dist1.pdf(x)*dist2.pdf(x);
        bins.emplace_back(x, p);
    }

    return make_pwl_product_dist(std::move(bins));
}

product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const gaussian1d_t& dist1,
                         const uniform_distribution_t& dist2) {
    constexpr auto pts_per_stddev = 6;
    constexpr auto stddevs = 5;
    std::vector<f_t> gxs;
    gxs.reserve((2*stddevs+1) * pts_per_stddev);
    for (int s=-stddevs; s<stddevs; ++s)
    for (int p=0; p<pts_per_stddev + (s==stddevs-1?1:0); ++p) {
        const auto x = dist1.mean() + dist1.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        gxs.emplace_back(x);
    }

    const auto xs = sorted_candidates(gxs, dist2.range());

    std::vector<vec2_t> bins;
    bins.reserve(xs.size());
    for (const auto x : xs) {
        if (!m::isfinite(x)) continue;

        const auto p = dist1.pdf(x)*dist2.pdf(x);
        bins.emplace_back(x, p);
    }

    return make_pwl_product_dist(std::move(bins));
}

product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const truncated_gaussian1d_t& dist1,
                         const uniform_distribution_t& dist2) {
    constexpr auto pts_per_stddev = 6;
    constexpr auto stddevs = 5;
    std::vector<f_t> gxs;
    gxs.reserve((2*stddevs+1) * pts_per_stddev);
    for (int s=-stddevs; s<stddevs; ++s)
    for (int p=0; p<pts_per_stddev + (s==stddevs-1?1:0); ++p) {
        const auto x = dist1.mean() + dist1.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        gxs.emplace_back(x);
    }

    const auto xs = sorted_candidates(gxs, dist2.range());

    std::vector<vec2_t> bins;
    bins.reserve(xs.size());
    for (const auto x : xs) {
        if (!m::isfinite(x)) continue;

        const auto p = dist1.pdf(x)*dist2.pdf(x);
        bins.emplace_back(x, p);
    }

    return make_pwl_product_dist(std::move(bins));
}


product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const gaussian1d_t& dist1,
                         const truncated_gaussian1d_t& dist2) {
    constexpr auto pts_per_stddev = 6;
    constexpr auto stddevs = 5;
    std::vector<f_t> gxs1, gxs2;
    gxs1.reserve((2*stddevs+1) * pts_per_stddev);
    gxs2.reserve((2*stddevs+1) * pts_per_stddev);
    for (int s=-stddevs; s<stddevs; ++s)
    for (int p=0; p<pts_per_stddev + (s==stddevs-1?1:0); ++p) {
        const auto x1 = dist1.mean() + dist1.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        const auto x2 = dist2.mean() + dist2.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        gxs1.emplace_back(x1);
        gxs2.emplace_back(x2);
    }

    const auto xs = sorted_candidates(gxs1, gxs2);

    std::vector<vec2_t> bins;
    bins.reserve(xs.size());
    for (const auto x : xs) {
        const auto p = dist1.pdf(x)*dist2.pdf(x);
        bins.emplace_back(x, p);
    }

    return make_pwl_product_dist(std::move(bins));
}

product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const truncated_gaussian1d_t& dist1,
                         const truncated_gaussian1d_t& dist2) {
    constexpr auto pts_per_stddev = 6;
    constexpr auto stddevs = 5;
    std::vector<f_t> gxs1, gxs2;
    gxs1.reserve((2*stddevs+1) * pts_per_stddev);
    gxs2.reserve((2*stddevs+1) * pts_per_stddev);
    for (int s=-stddevs; s<stddevs; ++s)
    for (int p=0; p<pts_per_stddev + (s==stddevs-1?1:0); ++p) {
        const auto x1 = dist1.mean() + dist1.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        const auto x2 = dist2.mean() + dist2.std_dev() * (f_t(s)+p/f_t(pts_per_stddev));
        gxs1.emplace_back(x1);
        gxs2.emplace_back(x2);
    }

    const auto xs = sorted_candidates(gxs1, gxs2);

    std::vector<vec2_t> bins;
    bins.reserve(xs.size());
    for (const auto x : xs) {
        const auto p = dist1.pdf(x)*dist2.pdf(x);
        bins.emplace_back(x, p);
    }

    return make_pwl_product_dist(std::move(bins));
}


product_distribution_ret_t<uniform_distribution_t>
    product_distribution(const uniform_distribution_t& dist1,
                         const uniform_distribution_t& dist2) {
    const auto& r1 = dist1.range();
    const auto& r2 = dist1.range();
    const auto overlap = r1 & r2;

    const f_t R0 = r1.length()>zero && r2.length()>zero ?
        overlap.length() / (r1.length() * r2.length()) :
        0;

    return {
        .dist = uniform_distribution_t{ dist1.range() & dist2.range() },
        .R0 = R0,
        .approximate = false,
    };
}

}
