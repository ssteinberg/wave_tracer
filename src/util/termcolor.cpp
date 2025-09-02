/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/util/logger/logger.hpp>
#include <wt/util/logger/termcolor.hpp>

namespace wt::logger::termcolour::_detail {

#ifdef __cpp_lib_syncstream
bool is_atty(std::osyncstream& stream) noexcept {
    const auto sb = stream.get_wrapped();
    if (!sb)
        return false;
    return sb==std::cout.rdbuf() || sb==std::cerr.rdbuf() || sb==std::clog.rdbuf();
}
#else
bool is_atty(basic_osyncstream& stream) noexcept {
    const auto sb = stream.get_wrapped();
    if (!sb)
        return false;
    return sb==std::cout.rdbuf() || sb==std::cerr.rdbuf() || sb==std::clog.rdbuf();
}
#endif

} // namespace wt::logger::termcolour::_detail

namespace wt::logger::termcolour {

#ifdef __cpp_lib_syncstream
bool is_colourized(std::osyncstream& stream) noexcept {
    return _detail::is_atty(stream);
}
#else
bool is_colourized(basic_osyncstream& stream) noexcept {
    return _detail::is_atty(stream);
}
#endif

} // namespace wt::logger::termcolour