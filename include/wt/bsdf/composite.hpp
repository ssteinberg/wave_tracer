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
#include <memory>
#include <map>

#include <wt/wt_context.hpp>

#include "bsdf.hpp"

namespace wt::bsdf {

/**
 * @brief A composition of one or more BSDFs, each defined over a distinct spectral range.
 *        Spectral ranges must not overlap.
 */
class composite_t final : public bsdf_t {
private:
    struct comparator_rangew_t {
        bool operator()(const auto& a, const auto& b) const {
            return a.max<=b.min;
        }
    };

    using map_range_t = range_t<wavenumber_t, range_inclusiveness_e::left_inclusive>;
    using map_t = std::map<map_range_t, 
                           std::shared_ptr<bsdf_t>, 
                           comparator_rangew_t>;

private:
    map_t bsdfs;
    range_t<wavenumber_t> range;

public:
    composite_t(std::string id, 
                map_t bsdfs) 
        : bsdf_t(std::move(id)), 
          bsdfs(std::move(bsdfs))
    {
        this->range = range_t<wavenumber_t>::null();
        for (const auto& s : this->bsdfs) {
            assert((this->range & s.first).empty());    // overlapping spectra?
            this->range |= s.first;
        }
    }
    composite_t(composite_t&&) = default;

    /**
     * @brief Spectral albedo. Returns std::nullopt when albedo cannot be computed.
     * @param k wavenumber
     */
    [[nodiscard]] inline std::optional<f_t> albedo(const wavenumber_t k) const noexcept override {
        const auto it = bsdfs.lower_bound(map_range_t::range(k));
        if (it!=bsdfs.end() && it->first.contains(k))
            return it->second->albedo(k);
        return 0;
    }
    
    /**
     * @brief Returns mask of all available lobes for this BSDF at particular wavenumber.
     */
    [[nodiscard]] lobe_mask_t lobes(wavenumber_t k) const noexcept override {
        const auto it = bsdfs.lower_bound(map_range_t::range(k));
        if (it!=bsdfs.end() && it->first.contains(k))
            return it->second->lobes(k);
        return 0;
    }
    
    [[nodiscard]] inline bool is_delta_only(wavenumber_t k) const noexcept override {
        const auto it = bsdfs.lower_bound(map_range_t::range(k));
        if (it!=bsdfs.end() && it->first.contains(k))
            return it->second->is_delta_only(k);
        return true;
    }
    
    /**
     * @brief Is a lobe a delta lobe?
     */
    [[nodiscard]] inline bool is_delta_lobe(wavenumber_t k, std::uint32_t lobe) const noexcept override {
        const auto it = bsdfs.lower_bound(map_range_t::range(k));
        if (it!=bsdfs.end() && it->first.contains(k))
            return it->second->is_delta_lobe(k, lobe);
        return true;
    }

    /**
     * @brief Evaluates the BSDF. Accounts for the cosine foreshortening term.
     */
    [[nodiscard]] bsdf_result_t f(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept override {
        const auto k  = query.k;
        const auto it = bsdfs.lower_bound(map_range_t::range(k));
        if (it!=bsdfs.end() && it->first.contains(k))
            return it->second->f(wi,wo,query);
        return {};
    }

    /**
     * @brief Samples the BSDF. The returned weight is bsdf/pdf. 
     * @param S normalized Stokes parameters vector that describes the polarimetric properties of incident radiation.
     */
    [[nodiscard]] std::optional<bsdf_sample_t> sample(
            const dir3_t &wi,
            const bsdf_query_t& query, 
            sampler::sampler_t& sampler) const noexcept override {
        const auto k  = query.k;
        const auto it = bsdfs.lower_bound(map_range_t::range(k));
        if (it!=bsdfs.end() && it->first.contains(k))
            return it->second->sample(wi,query,sampler);
        return {};
    }
    
    /**
     * @brief Provides the sample density. 
     * @param S normalized Stokes parameters vector that describes the polarimetric properties of incident radiation.
     */
    [[nodiscard]] solid_angle_density_t pdf(
            const dir3_t &wi,
            const dir3_t &wo,
            const bsdf_query_t& query) const noexcept override {
        const auto k  = query.k;
        const auto it = bsdfs.lower_bound(map_range_t::range(k));
        if (it!=bsdfs.end() && it->first.contains(k))
            return it->second->pdf(wi,wo,query);
        return {};
    }

    /**
     * @brief Computes the refractive-index ratio: eta at exit / eta at entry.
     * @param k wavenumber
     */
    [[nodiscard]] f_t eta(
            const dir3_t &wi,
            const dir3_t &wo,
            const wavenumber_t k) const noexcept override {
        const auto it = bsdfs.lower_bound(map_range_t::range(k));
        if (it!=bsdfs.end() && it->first.contains(k))
            return it->second->eta(wi,wo,k);
        return {};
    }

    [[nodiscard]] scene::element::info_t description() const override;

public:
    static std::unique_ptr<bsdf_t> load(std::string id, scene::loader::loader_t* loader, const scene::loader::node_t& node, const wt::wt_context_t &context);
};

}
