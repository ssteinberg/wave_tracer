/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <sstream>
#include <format>

#include <cassert>

#include <wt/version.hpp>
#include <wt/util/logger/logger.hpp>

#include <GitHash.hpp>


using namespace wt;

std::string wt_version_t::short_version_string() {
	return std::format("{:d}",major()) + "." + std::format("{:d}",minor());
}

std::string wt_version_t::full_version_string() {
	return short_version_string() + "." + std::format("{:d}",patch())
			+ " #" + build_string();
}

void wt_version_t::print_wt_version() {
    using namespace logger::termcolour;
	wt::logger::cout(verbosity_e::important)
		<< bold << yellow << " 〜 wave_tracer " 
			<< std::format("{:d}",major()) << "." << std::format("{:d}",minor()) << "." << std::format("{:d}",patch())
			<< italic << yellow << " #" << build_string()
		<< reset << "   by  Shlomi Steinberg" 
		<< '\n';
}

wt_version_t::wt_version_t() {
    std::stringstream ss(WTVERSION);

    ss >> mjr;
    assert (ss.peek() == '.');
	ss.ignore();
	ss >> mnr;
    assert (ss.peek() == '.');
	ss.ignore();
	ss >> ptch;

	build_str = std::string{ GitHash::shortSha1 } + (GitHash::dirty ? "-dirty" : "");
}
