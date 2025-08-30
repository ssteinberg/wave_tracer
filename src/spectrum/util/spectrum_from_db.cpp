/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/


#include <algorithm>
#include <vector>
#include <sstream>
#include <string>

#include <wt/spectrum/util/spectrum_from_db.hpp>

#include <wt/math/common.hpp>
#include <wt/math/range.hpp>

#include <wt/wt_context.hpp>
#include <wt/util/logger/logger.hpp>

#include <wt/spectrum/binned.hpp>
#include <wt/spectrum/piecewise_linear.hpp>

#include <yaml-cpp/yaml.h>

using namespace wt;
using namespace spectrum;


template <Quantity C>
inline auto data_from_Sellmeier_eq(
        const range_t<wavelength_t>& lambda_range,
        const f_t scale, 
        f_t A, f_t B1, C C1, f_t B2, C C2, f_t B3, C C3, 
        const wavelength_t& lambda_step = 5*u::nm) noexcept {
    const auto steps_fp = (f_t)(m::max<wavelength_t>(0*u::m,lambda_range.length())/lambda_step);
    const auto steps = m::max<std::size_t>(2,(std::size_t)m::ceil(steps_fp));
    const auto recp_steps = f_t(1)/(steps-1);

    std::vector<vec2_t> data;
    data.resize(steps);

    for (std::size_t i=0; i<steps; ++i) {
        const auto l = m::mix(lambda_range, i*recp_steps);
        const auto l2 = m::sqr(l);
        const auto v = m::sqrt(
            1 + A + f_t(B1*l2/(l2-C1)) + f_t(B2*l2/(l2-C2)) + f_t(B3*l2/(l2-C3))
        );

        data[i] = { u::to_inv_mm(wavelen_to_wavenum(l)), v * scale };
    }

    return data;
}

detail_spectrum_from_db::spectrum_from_material_ret_t detail_spectrum_from_db::spectrum_from_db(
        const wt_context_t& ctx,
        const std::string& name,
        const std::uint16_t count,
        const wavelength_t db_wavelength,
        const f_t scale,
        const bool bin) {
    const auto path = ctx.resolve_path(std::filesystem::path{ "data" } / (name+".yml"));
    if (!path)
        throw std::runtime_error("(spectrum_from_db) Spectrum database \"" + name + "\" not found.");

    std::vector<std::vector<vec2_t>> channels;
    channels.resize(count);
    std::vector<f_t> items;
    items.resize(count);

    // parse YAML
    try {
        const auto db = YAML::LoadFile(path->string());
        if (!db)
            throw std::runtime_error("(spectrum_from_db) Loading spectrum database \"" + name + "\" failed.");

        // might consist of different database entries, parse each
        std::uint16_t c=0;
        const auto dbs = db["DATA"];
        for (YAML::const_iterator it=dbs.begin();it!=dbs.end();++it) {
            const auto type = (*it)["type"].as<std::string>();
            if (type == "formula 1" || type == "formula 2") {
                // Sellmeier equation for dielectric IORs
                // read coefficients
                f_t A,B1,C1,B2,C2,B3,C3;
                auto ss = std::istringstream{ (*it)["coefficients"].as<std::string>() };
                ss >> A >> B1 >> C1 >> B2 >> C2 >> B3 >> C3;
                // read range
                ss = std::istringstream{ (*it)["wavelength_range"].as<std::string>() };
                f_t l1,l2;
                ss >> l1 >> l2;
                range_t<wavelength_t> ls = { .min=l1*db_wavelength, .max=l2*db_wavelength };

                if (type == "formula 1") {
                    C1 = m::sqr(C1);
                    C2 = m::sqr(C2);
                    C3 = m::sqr(C3);
                }

                channels[c++] = data_from_Sellmeier_eq(
                        ls, scale, 
                        A, B1, C1*m::sqr(db_wavelength), 
                           B2, C2*m::sqr(db_wavelength), 
                           B3, C3*m::sqr(db_wavelength));
                assert(channels[c-1].size());
            }
            else if (type.starts_with("tabulated ")) {
                // tabulated values
                const auto data_str = (*it)["data"].as<std::string>();
                std::istringstream istr(data_str);
                std::string line;
                while (std::getline(istr, line)) {
                    auto ss = std::istringstream{ line };

                    f_t wl=0, v;
                    ss >> wl;
                    int idx=0;
                    for (;idx<(int)count-(int)c && !ss.eof();++idx) {
                        ss >> v;
                        items[idx]=v;
                    }

                    if (wl<=0)
                        throw std::runtime_error("(spectrum_from_db) Loading spectrum database \"" + name + "\" failed.");

                    const auto k = wavelen_to_wavenum(wl * db_wavelength);
                    for (std::size_t i=0;i<idx;++i)
                        channels[c+i].emplace_back(u::to_inv_mm(k), items[i] * scale);
                }
            }
            else {
                assert(false);
                logger::cerr(verbosity_e::important) << "(spectrum_from_db) Unsupported database entry type '" + type + "'" << '\n';
            }
        }
    } catch(YAML::ParserException& exp) {
        throw std::runtime_error("(spectrum_from_db) Parsing failed: " + std::string{ exp.what() });
    }

    for (auto& c : channels) {
        if (c.size()<2) {
            // invalid spectrum
            // add dummy 0 values
            // (e.g., empty complex part)
            c.resize(2);
            c[0] = { 0,0 };
            c[1] = { 1,0 };
        }
    }

    spectrum_from_material_ret_t ret;
    ret.id = name;
    ret.channels.reserve(channels.size());
    for (std::size_t c=0;c<channels.size();++c) {
        auto& chnl = channels[c];
        assert(chnl.size()>=2);

        // add an explicit 0 value before and after the spectrum range
        // this creates a smooth distribution that works better with distributions sums and products
        chnl.emplace_back(m::max<f_t>(0,m::mix(chnl[chnl.size()-2].x,chnl[chnl.size()-1].x,1.01)), 0);
        std::ranges::reverse(chnl);
        chnl.emplace_back(m::max<f_t>(0,m::mix(chnl[chnl.size()-2].x,chnl[chnl.size()-1].x,1.01)), 0);

        auto pwld = piecewise_linear_distribution_t{ std::move(chnl) };

        if (!bin) {
            ret.channels.emplace_back(std::make_unique<piecewise_linear_t>(
                channels.size()>1 ? name+"_channel"+std::format("{:d}",c) : name, 
                std::move(pwld)
            ));
        } else {
            auto bpwld = binned_piecewise_linear_distribution_t{ pwld };
            // sanity: check that total power after binning is within 1%
            assert(pwld.total()>0 && m::abs(bpwld.total() - pwld.total()) / pwld.total() < .01);

            ret.channels.emplace_back(std::make_unique<binned_t>(
                channels.size()>1 ? name+"_channel"+std::format("{:d}",c) : name, 
                std::move(bpwld)
            ));
        }
    }

    return ret;
}
