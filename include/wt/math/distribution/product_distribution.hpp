/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <cassert>
#include <utility>
#include <wt/util/unreachable.hpp>

#include <wt/math/common.hpp>
#include "discrete_distribution.hpp"
#include "gaussian1d.hpp"
#include "truncated_gaussian1d.hpp"
#include "piecewise_linear_distribution.hpp"
#include "binned_piecewise_linear_distribution.hpp"
#include "uniform_distribution.hpp"

namespace wt {

template <typename Dist>
struct product_distribution_ret_t {
    using dist_type = Dist;

    /** 
     * @brief The computed product distribution \f$ h(x) = \frac{ f(x)g(x) }{ R_0 } \f$, where \f$ R_0 \f$ is the normalization factor.
     *        ``dist`` will always be one of the following:
     *        1. `wt::discrete_distribution_t<wt::vec2_t>`
     *        2. `wt::piecewise_linear_distribution_t`
     *        3. `wt::uniform_distribution_t`
     */
    Dist dist;

    /**
     * @brief The normalization factor, i.e. the functional norm of the product of the densities: \f$ R_0 = \int \mathrm dx f(x)g(x) \f$.
     *        The factor \f$ 0 \leq R_0 \leq 1 \f$ can be understood as the correlation coefficient (cross-correlation, at zero lag, between the distributions), quantifying *similarity between the distributions*.
     */
    f_t R0;

    /** @brief Flag that indicates that the computed product distribution is approximative. */
    bool approximate;

    /** @brief Flag that indicates that the product distribution `dist` and \f$ R_0 \f$ are discrete (iff both input distributions are discrete). */
    bool discrete = false;
};


/**
 * @brief Returns the product distribution of two independent discrete distributions.
 */
product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& dist1,
                         const discrete_distribution_t<vec2_t>& dist2);

/**
 * @brief Returns the product distribution of independent discrete and uniform distributions.
 */
product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& dist1,
                         const uniform_distribution_t& dist2);
/**
 * @brief Returns the product distribution of independent discrete and uniform distributions.
 */
inline product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const uniform_distribution_t& dist1,
                         const discrete_distribution_t<vec2_t>& dist2) {
    return product_distribution(dist2, dist1);
}

/**
 * @brief Returns the product distribution of independent discrete and piecewise-linear distributions.
 */
product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& dist1,
                         const piecewise_linear_distribution_t& dist2);
/**
 * @brief Returns the product distribution of independent discrete and piecewise-linear distributions.
 */
inline product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const piecewise_linear_distribution_t& dist1,
                         const discrete_distribution_t<vec2_t>& dist2) {
    return product_distribution(dist2, dist1);
}

/**
 * @brief Returns the product distribution of independent discrete and piecewise-linear distributions.
 */
product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& dist1,
                         const binned_piecewise_linear_distribution_t& dist2);
/**
 * @brief Returns the product distribution of independent discrete and piecewise-linear distributions.
 */
inline product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const binned_piecewise_linear_distribution_t& dist1,
                         const discrete_distribution_t<vec2_t>& dist2) {
    return product_distribution(dist2, dist1);
}

/**
 * @brief Returns the product distribution of independent discrete and Gaussian distributions.
 */
product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& dist1,
                         const gaussian1d_t& dist2);
/**
 * @brief Returns the product distribution of independent discrete and Gaussian distributions.
 */
inline product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const gaussian1d_t& dist1,
                         const discrete_distribution_t<vec2_t>& dist2) {
    return product_distribution(dist2, dist1);
}

/**
 * @brief Returns the product distribution of independent discrete and Gaussian distributions.
 */
product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const discrete_distribution_t<vec2_t>& dist1,
                         const truncated_gaussian1d_t& dist2);
/**
 * @brief Returns the product distribution of independent discrete and Gaussian distributions.
 */
inline product_distribution_ret_t<discrete_distribution_t<vec2_t>>
    product_distribution(const truncated_gaussian1d_t& dist1,
                         const discrete_distribution_t<vec2_t>& dist2) {
    return product_distribution(dist2, dist1);
}


/**
 * @brief Returns the product distribution of two independent piecewise-linear distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const piecewise_linear_distribution_t& dist1,
                         const piecewise_linear_distribution_t& dist2);

/**
 * @brief Returns the product distribution of independent piecewise-linear and uniform distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const piecewise_linear_distribution_t& dist1,
                         const uniform_distribution_t& dist2);
/**
 * @brief Returns the product distribution of independent piecewise-linear and uniform distributions.
 */
inline product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const uniform_distribution_t& dist1,
                         const piecewise_linear_distribution_t& dist2) {
    return product_distribution(dist2, dist1);
}

/**
 * @brief Returns the product distribution of independent piecewise-linear and Gaussian distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const piecewise_linear_distribution_t& dist1,
                         const gaussian1d_t& dist2);
/**
 * @brief Returns the product distribution of independent piecewise-linear and Gaussian distributions.
 */
inline product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const gaussian1d_t& dist1,
                         const piecewise_linear_distribution_t& dist2) {
    return product_distribution(dist2, dist1);
}

/**
 * @brief Returns the product distribution of independent piecewise-linear and Gaussian distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const piecewise_linear_distribution_t& dist1,
                         const truncated_gaussian1d_t& dist2);
/**
 * @brief Returns the product distribution of independent piecewise-linear and Gaussian distributions.
 */
inline product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const truncated_gaussian1d_t& dist1,
                         const piecewise_linear_distribution_t& dist2) {
    return product_distribution(dist2, dist1);
}

/**
 * @brief Returns the product distribution of two independent piecewise-linear distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const piecewise_linear_distribution_t& dist1,
                         const binned_piecewise_linear_distribution_t& dist2);
/**
 * @brief Returns the product distribution of two independent piecewise-linear distributions.
 */
inline product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const binned_piecewise_linear_distribution_t& dist1,
                         const piecewise_linear_distribution_t& dist2) {
    return product_distribution(dist2, dist1);
}


/**
 * @brief Returns the product distribution of two independent binned piecewise-linear distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const binned_piecewise_linear_distribution_t& dist1,
                         const binned_piecewise_linear_distribution_t& dist2);

/**
 * @brief Returns the product distribution of independent binned piecewise-linear and uniform distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const binned_piecewise_linear_distribution_t& dist1,
                         const uniform_distribution_t& dist2);
/**
 * @brief Returns the product distribution of independent binned piecewise-linear and uniform distributions.
 */
inline product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const uniform_distribution_t& dist1,
                         const binned_piecewise_linear_distribution_t& dist2) {
    return product_distribution(dist2, dist1);
}

/**
 * @brief Returns the product distribution of independent binned piecewise-linear and Gaussian distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const binned_piecewise_linear_distribution_t& dist1,
                         const gaussian1d_t& dist2);
/**
 * @brief Returns the product distribution of independent binned piecewise-linear and Gaussian distributions.
 */
inline product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const gaussian1d_t& dist1,
                         const binned_piecewise_linear_distribution_t& dist2) {
    return product_distribution(dist2, dist1);
}

/**
 * @brief Returns the product distribution of independent binned piecewise-linear and Gaussian distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const binned_piecewise_linear_distribution_t& dist1,
                         const truncated_gaussian1d_t& dist2);
/**
 * @brief Returns the product distribution of independent binned piecewise-linear and Gaussian distributions.
 */
inline product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const truncated_gaussian1d_t& dist1,
                         const binned_piecewise_linear_distribution_t& dist2) {
    return product_distribution(dist2, dist1);
}


/**
 * @brief Returns the product distribution of two independent Gaussian distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const gaussian1d_t& dist1,
                         const gaussian1d_t& dist2);

/**
 * @brief Returns the product distribution of independent Gaussian and uniform distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const gaussian1d_t& dist1,
                         const uniform_distribution_t& dist2);
/**
 * @brief Returns the product distribution of independent Gaussian and uniform distributions.
 */
inline product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const uniform_distribution_t& dist1,
                         const gaussian1d_t& dist2) {
    return product_distribution(dist2, dist1);
}

/**
 * @brief Returns the product distribution of independent Gaussian and uniform distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const gaussian1d_t& dist1,
                         const truncated_gaussian1d_t& dist2);
/**
 * @brief Returns the product distribution of independent Gaussian and uniform distributions.
 */
inline product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const truncated_gaussian1d_t& dist1,
                         const gaussian1d_t& dist2) {
    return product_distribution(dist2, dist1);
}


/**
 * @brief Returns the product distribution of two independent Gaussian distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const truncated_gaussian1d_t& dist1,
                         const truncated_gaussian1d_t& dist2);

/**
 * @brief Returns the product distribution of independent Gaussian and uniform distributions.
 */
product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const truncated_gaussian1d_t& dist1,
                         const uniform_distribution_t& dist2);
/**
 * @brief Returns the product distribution of independent Gaussian and uniform distributions.
 */
inline product_distribution_ret_t<piecewise_linear_distribution_t>
    product_distribution(const uniform_distribution_t& dist1,
                         const truncated_gaussian1d_t& dist2) {
    return product_distribution(dist2, dist1);
}


/**
 * @brief Returns the product distribution of two independent uniform distributions.
 */
product_distribution_ret_t<uniform_distribution_t>
    product_distribution(const uniform_distribution_t& dist1,
                         const uniform_distribution_t& dist2);


namespace detail {

template <typename T, typename S>
inline auto product_distribution_impl_b(const T* f, const S* g) {
    auto d = product_distribution(*f, *g);
    return product_distribution_ret_t<std::unique_ptr<distribution1d_t>>{
        .dist = std::make_unique<typename decltype(d)::dist_type>(std::move(d.dist)),
        .R0 = d.R0,
        .approximate = d.approximate,
        .discrete = d.discrete,
    };
}

template <typename T>
inline auto product_distribution_impl_a(const T* f, const distribution1d_t* g) {
    if (auto p = dynamic_cast<const discrete_distribution_t<vec2_t>*>(g); p)
        return detail::product_distribution_impl_b(f, p);
    if (auto p = dynamic_cast<const uniform_distribution_t*>(g); p)
        return detail::product_distribution_impl_b(f, p);
    if (auto p = dynamic_cast<const piecewise_linear_distribution_t*>(g); p)
        return detail::product_distribution_impl_b(f, p);
    if (auto p = dynamic_cast<const binned_piecewise_linear_distribution_t*>(g); p)
        return detail::product_distribution_impl_b(f, p);
    if (auto p = dynamic_cast<const gaussian1d_t*>(g); p)
        return detail::product_distribution_impl_b(f, p);
    if (auto p = dynamic_cast<const truncated_gaussian1d_t*>(g); p)
        return detail::product_distribution_impl_b(f, p);

    unreachable();
}

}


/**
 * @brief Computes the normalized product distribution of two independent distributions: \f$ h(x) = \frac{ f(x)g(x) }{ R_0 } \f$, where \f$ f,g \f$ are the input PDFs, and \f$ R_0 = \int \mathrm dx f(x)g(x) \f$ is the normalization factor.
 *        The returned distribution will always be one of the following:
 *        1. `wt::discrete_distribution_t<wt::vec2_t>`
 *        2. `wt::piecewise_linear_distribution_t`
 *        3. `wt::uniform_distribution_t`
 *
 *        Depending on the input, the computed distribution may not be exact.
 *        Also returns the normalization factor \f$ R_0 \f$. See `wt::product_distribution_ret_t`.
 */
inline product_distribution_ret_t<std::unique_ptr<distribution1d_t>>
    product_distribution(const distribution1d_t* f,
                         const distribution1d_t* g) {
    if (auto p = dynamic_cast<const discrete_distribution_t<vec2_t>*>(f); p)
        return detail::product_distribution_impl_a(p, g);
    if (auto p = dynamic_cast<const uniform_distribution_t*>(f); p)
        return detail::product_distribution_impl_a(p, g);
    if (auto p = dynamic_cast<const piecewise_linear_distribution_t*>(f); p)
        return detail::product_distribution_impl_a(p, g);
    if (auto p = dynamic_cast<const binned_piecewise_linear_distribution_t*>(f); p)
        return detail::product_distribution_impl_a(p, g);
    if (auto p = dynamic_cast<const gaussian1d_t*>(f); p)
        return detail::product_distribution_impl_a(p, g);
    if (auto p = dynamic_cast<const truncated_gaussian1d_t*>(f); p)
        return detail::product_distribution_impl_a(p, g);

    unreachable();
}

}
