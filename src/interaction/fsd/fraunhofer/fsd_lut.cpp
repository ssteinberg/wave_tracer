/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <future>
#include <stdexcept>

#include <wt/interaction/fsd/fraunhofer/fsd_lut.hpp>

#include <wt/math/common.hpp>

using namespace wt;
using namespace wt::fraunhofer;
using namespace wt::fraunhofer::fsd_sampler;


// FSD LUT loading

template <typename Data, typename SrcT = double, typename DstT = f_t>
inline auto load_csv(Data &data, const std::filesystem::path &path) {
    static_assert(sizeof(Data)%sizeof(DstT)==0);
    constexpr auto size = sizeof(Data)/sizeof(DstT);

    std::ifstream f(path, std::ios::in | std::ios::binary);
    if (f.bad() || f.fail())
        throw std::runtime_error("(fsd_lut) Could not open \"" + path.string() + "\"");

    std::vector<char> content;
    content.reserve(sizeof(data));
    content.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());

    if (content.size()%sizeof(SrcT)!=0 || content.size()/sizeof(SrcT)!=size)
        throw std::runtime_error("(fsd_lut) Mismatched size of LUT \"" + path.string() + "\".");

    auto *src = reinterpret_cast<const SrcT*>(content.data());
    auto *dst = reinterpret_cast<DstT*>(&data);
    for (auto *p = dst; p!=dst+size; ++p, ++src)
        *p = DstT(*src);
}

fsd_lut_t::fsd_lut_t(const wt::wt_context_t &context) {
    const auto data_path = std::filesystem::path{ "data" } / "fsd";

    data = std::make_unique<data_t>();

    const auto path_a1  = context.resolve_path(data_path / "iCDFa1.fp64");
    const auto path_a2  = context.resolve_path(data_path / "iCDFa2.fp64");
    const auto path_a1t = context.resolve_path(data_path / "iCDFa1theta.fp64");
    const auto path_a2t = context.resolve_path(data_path / "iCDFa2theta.fp64");

    if (!path_a1 || !path_a2 || !path_a1t || !path_a2t)
        throw std::runtime_error("(fsd_lut) FSD LUTs not found.");
    
    auto f1 = std::async(std::launch::async, [&]() {
        load_csv(data->iCDF1, *path_a1);
    });
    auto f2 = std::async(std::launch::async, [&]() {
        load_csv(data->iCDF2, *path_a2);
    });
    load_csv(data->iCDFtheta1, *path_a1t);
    load_csv(data->iCDFtheta2, *path_a2t);

    f1.get();
    f2.get();
}
