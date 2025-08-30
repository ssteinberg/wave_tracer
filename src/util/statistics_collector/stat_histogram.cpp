/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 * Authors:  Umut Emre, Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#include <string>
#include <array>

#include <wt/math/common.hpp>
#include <wt/util/statistics_collector/stat_histogram.hpp>

#include <wt/util/logger/logger.hpp>

#define TINYCOLORMAP_WITH_GLM
#include <tinycolormap.hpp>

using namespace wt;
using namespace wt::stats;


void stat_histogram::pretty_print_histogram(std::ostream& os,
                                            const std::string& label,
                                            const std::function<std::string(std::size_t)>& bin_to_label,
                                            const std::size_t* values,
                                            std::size_t count,
                                            const stats_t& stats) {
    static constexpr std::size_t H = 3;
    static constexpr std::size_t B = 9;
    const std::string bars[B] = { " ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█" };
    const std::string empty = "_";

    constexpr auto colourmap = tinycolormap::ColormapType::Cividis;

    const bool have_uf = values[0]>0;
    const bool have_of = values[count-1]>0;
    if (!have_uf) {
        values = values+1; --count;
    }
    if (!have_of) --count;

    const auto min_size = 8ul;
    constexpr auto max_bins = 50ul;

    // trim tail zeros
    std::size_t non_zero_count = 0;
    for (auto b=1ul;b<count-1;++b)
        if (values[b]>0) non_zero_count = b;
    non_zero_count = m::max(min_size, non_zero_count);
    if (non_zero_count<count-2) ++non_zero_count;     // have an empty bin at then end, for visual purposes

    const auto count_without_uo = non_zero_count;
    const auto of_idx = count-1;
    count = count_without_uo + int(have_of)+int(have_uf);

    // if needed, shrink values into a smaller histogram, accumulating several bins into one
    std::array<std::size_t, max_bins> rebuilt_values;
    if (count > max_bins) {
        const auto mb = max_bins-int(have_of)-int(have_uf);
        const auto s = (count_without_uo + mb-1)/mb;
        const auto new_count = count_without_uo/s + int(have_of)+int(have_uf);

        rebuilt_values.fill(0);
        auto i=0ul;
        if (have_uf) rebuilt_values[i++] = values[0];
        for (;i<new_count-int(have_of);++i)
        for (auto j=0ul;j<s;++j) {
            const auto b = (i-int(have_uf))*s+j + int(have_uf);
            rebuilt_values[i] += b<count-1 ? values[b] : 0;
        }
        if (have_of) rebuilt_values[i++] = values[of_idx];
        assert(i==new_count);

        values = rebuilt_values.data();
        count = new_count;
    } else {
        std::copy(values, values+count-1, rebuilt_values.data());
        rebuilt_values[count-1] = values[of_idx];

        values = rebuilt_values.data();
    }

    // build lines
    std::size_t max = 0;
    for (auto i=0ul;i<count;++i) max = m::max(max,values[i]);

    // build lower ticks/legend bar
    auto start   = bin_to_label(0);
    auto lbl_1_4 = bin_to_label((count_without_uo+2)/4);
    auto lbl_2_4 = bin_to_label(count_without_uo/2);
    auto lbl_3_4 = bin_to_label(count_without_uo*3/4);
    auto end     = bin_to_label(count_without_uo);

    if      (lbl_1_4.ends_with(".50")) lbl_1_4.erase(lbl_1_4.size()-1);
    else if (lbl_1_4.ends_with(".00")) lbl_1_4.erase(lbl_1_4.size()-3);
    if      (lbl_2_4.ends_with(".50")) lbl_2_4.erase(lbl_2_4.size()-1);
    else if (lbl_2_4.ends_with(".00")) lbl_2_4.erase(lbl_2_4.size()-3);
    if      (lbl_3_4.ends_with(".50")) lbl_3_4.erase(lbl_3_4.size()-1);
    else if (lbl_3_4.ends_with(".00")) lbl_3_4.erase(lbl_3_4.size()-3);

    const auto row_max_len = count;
    const auto luf = have_uf ? std::string{ "<" } : "";
    const auto lof = have_of ? std::string{ ">" } : "";
    const auto& ll = start;
    const auto& lr = end;
    const auto l_2_4 = row_max_len > ll.length() + lr.length() + luf.length() + lof.length() + lbl_2_4.length() + 2 ?
                    " " + lbl_2_4 + " " : "";
    const bool qs = 
        row_max_len >= ll.length() + lr.length() + luf.length() + lof.length() + l_2_4.length() + lbl_1_4.length() + lbl_3_4.length() + 4;
    const auto l_1_4 = qs ? " " + lbl_1_4 + " " : "";
    const auto l_3_4 = qs ? " " + lbl_3_4 + " " : "";
    const auto spaces = 
        row_max_len - m::min(row_max_len, l_1_4.length() + l_2_4.length() + l_3_4.length() + ll.length() + lr.length() + luf.length() + lof.length());
    const auto sep1 = std::string((spaces+2)/4, ' ');
    const auto sep2 = std::string((spaces+1)/4, ' ');
    const auto sep3 = std::string((spaces)/4, ' ');
    const auto sep4 = std::string((spaces+3)/4, ' ');

    // name label
    constexpr std::size_t labelw = stat_collector_t::name_label_maxw-1;
    auto name = std::string(stat_collector_t::print_indent, ' ') + label;
    if (name.length() > labelw) name = name.substr(0,labelw-1) + ' ';
    if (name.length() < labelw) name.append(labelw-name.length(), ' ');
    name += '\t';
    const auto line_indent = std::string(labelw, ' ') + '\t';


    using namespace logger::termcolour;

    // print
    for (auto l=0ul;l<H;++l) {
        os 
           << reset << bright_white
           << (l==H-(H/2) ? name : line_indent) << ' '
           << reset;

        for (auto i=0ul;i<count;++i) {
            const auto f = max==0 ? 0 : values[i]==max ? f_t(1) : values[i]/f_t(max);
            auto v = f==1 ? (B-1)*H : (std::size_t)(f*(B-1)*H + f_t(.5));
            for (auto ll=0ul;ll<=H-l-1;++ll) {
                if (ll==H-l-1) {
                    const auto colour = tinycolormap::GetColor(f, colourmap).ConvertToGLM();
                    const auto c = v==0&&ll==0 ? empty : bars[m::min(B-1,v)];
                    os << logger::termcolour::rgb_colour_t
                        { std::uint8_t(colour.x*255),std::uint8_t(colour.y*255),std::uint8_t(colour.z*255) }
                       << c;
                }
                v -= m::min(v,B-1);
            }
        }

        // stats
        if (l==2) {
           os << "    " << reset
              << dark << white << "mean "
              << reset << blue << bold
              << (std::size_t)stats.mean
              << reset << cyan << " ± "
              << std::setprecision(2)
              << stats.stddev;
        }

        os << reset << '\n';
    }

    os
        << line_indent << ' '
		<< reset << dark << white
        << luf << ll << sep1 << l_1_4 << sep2 << l_2_4 << sep3 << l_3_4 << sep4 << lr << lof
        << reset
        << '\n';
}
