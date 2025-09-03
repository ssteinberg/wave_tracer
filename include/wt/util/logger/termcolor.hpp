/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#ifdef WIN32
#include <stdio.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include <cassert>
#include <cstdint>
#include <iostream>
#ifdef __cpp_lib_syncstream
#include <syncstream>
#endif

namespace wt::logger {

// Forward declarations
#ifndef __cpp_lib_syncstream
class basic_osyncstream;
#endif

//
// based on github.com/p-ranav/indicators by Pranav
// (MIT)
// with very minor modifications
//

enum class colour : std::uint8_t
{ grey, red, green, yellow, blue, magenta, cyan, white, unspecified };

enum class font_style : std::uint8_t
{ bold, dark, italic, underline, blink, reverse, concealed, crossed };


namespace termcolour {

namespace _detail
{

inline int colourize_index() noexcept;
inline FILE* get_standard_stream(const std::ostream& stream) noexcept;
inline bool is_atty(const std::ostream& stream) noexcept;

}

inline bool is_colourized(std::ostream& stream) noexcept;

inline std::ostream& colourize(std::ostream& stream) noexcept {
    stream.iword(_detail::colourize_index()) = 1L;
    return stream;
}

inline std::ostream& nocolourize(std::ostream& stream) noexcept {
    stream.iword(_detail::colourize_index()) = 0L;
    return stream;
}

inline std::ostream& reset(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[00m";
    return stream;
}

inline std::ostream& bold(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[1m";
    return stream;
}

inline std::ostream& dark(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[2m";
    return stream;
}

inline std::ostream& italic(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[3m";
    return stream;
}

inline std::ostream& underline(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[4m";
    return stream;
}

inline std::ostream& blink(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[5m";
    return stream;
}

inline std::ostream& reverse(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[7m";
    return stream;
}

inline std::ostream& concealed(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[8m";
    return stream;
}

inline std::ostream& crossed(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[9m";
    return stream;
}

template <std::uint8_t code>
inline std::ostream& colour(std::ostream& stream) noexcept {
    if (is_colourized(stream)) {
        char command[12];
        std::snprintf(command, sizeof(command), "\033[38;5;%dm", code);
        stream << command;
    }
    return stream;
}

template <std::uint8_t code>
inline std::ostream& on_colour(std::ostream& stream) noexcept {
    if (is_colourized(stream)) {
        char command[12];
        std::snprintf(command, sizeof(command), "\033[48;5;%dm", code);
        stream << command;
    }
    return stream;
}

struct rgb_colour_t {
    std::uint8_t r; std::uint8_t g; std::uint8_t b;
};

inline std::ostream& operator<<(std::ostream& stream, rgb_colour_t c) noexcept {
    if (is_colourized(stream)) {
        char command[20];
        std::snprintf(command, sizeof(command), "\033[38;2;%d;%d;%dm", c.r, c.g, c.b);
        stream << command;
    }
    return stream;
}

inline std::ostream& grey(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[30m";
    return stream;
}

inline std::ostream& red(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[31m";
    return stream;
}

inline std::ostream& green(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[32m";
    return stream;
}

inline std::ostream& yellow(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[33m";
    return stream;
}

inline std::ostream& blue(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[34m";
    return stream;
}

inline std::ostream& magenta(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[35m";
    return stream;
}

inline std::ostream& cyan(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[36m";
    return stream;
}

inline std::ostream& white(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[37m";
    return stream;
}


inline std::ostream& bright_grey(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[90m";
    return stream;
}

inline std::ostream& bright_red(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[91m";
    return stream;
}

inline std::ostream& bright_green(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[92m";
    return stream;
}

inline std::ostream& bright_yellow(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[93m";
    return stream;
}

inline std::ostream& bright_blue(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[94m";
    return stream;
}

inline std::ostream& bright_magenta(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[95m";
    return stream;
}

inline std::ostream& bright_cyan(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[96m";
    return stream;
}

inline std::ostream& bright_white(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[97m";
    return stream;
}


inline std::ostream& on_grey(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[40m";
    return stream;
}

inline std::ostream& on_red(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[41m";
    return stream;
}

inline std::ostream& on_green(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[42m";
    return stream;
}

inline std::ostream& on_yellow(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[43m";
    return stream;
}

inline std::ostream& on_blue(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[44m";
    return stream;
}

inline std::ostream& on_magenta(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[45m";
    return stream;
}

inline std::ostream& on_cyan(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[46m";
    return stream;
}

inline std::ostream& on_white(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[47m";

    return stream;
}


inline std::ostream& on_bright_grey(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[100m";
    return stream;
}

inline std::ostream& on_bright_red(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[101m";
    return stream;
}

inline std::ostream& on_bright_green(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[102m";
    return stream;
}

inline std::ostream& on_bright_yellow(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[103m";
    return stream;
}

inline std::ostream& on_bright_blue(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[104m";
    return stream;
}

inline std::ostream& on_bright_magenta(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[105m";
    return stream;
}

inline std::ostream& on_bright_cyan(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[106m";
    return stream;
}

inline std::ostream& on_bright_white(std::ostream& stream) noexcept {
    if (is_colourized(stream))
        stream << "\033[107m";

    return stream;
}


namespace _detail
{

// An index to be used to access a private storage of I/O streams. See
// colourize / nocolourize I/O manipulators for details. Due to the fact
// that static variables ain't shared between translation units, inline // function with local static variable is used to do the trick and share
// the variable value between translation units.
inline int colourize_index() noexcept {
    static int colourize_index = std::ios_base::xalloc();
    return colourize_index;
}

//! Since C++ hasn't a true way to extract stream handler
//! from the a given `std::ostream` object, I have to write
//! this kind of hack.
inline FILE* get_standard_stream(const std::ostream& stream) noexcept {
    if (&stream == &std::cout)
        return stdout;
    else if ((&stream == &std::cerr) || (&stream == &std::clog))
        return stderr;

    return nullptr;
}

//! Test whether a given `std::ostream` object refers to
//! a terminal.
inline bool is_atty(const std::ostream& stream) noexcept {
    FILE* std_stream = get_standard_stream(stream);
    if (!std_stream)
        return false;

#ifdef WIN32
    auto fd = _fileno(std_stream); 
    return _isatty(fd);
#else
    return ::isatty(fileno(std_stream));
#endif
}
// Forward declarations for non-inline functions
#ifdef __cpp_lib_syncstream
bool is_atty(std::osyncstream& stream) noexcept;
#else
bool is_atty(basic_osyncstream& stream) noexcept;
#endif

}

/**
 * @brief Manually mark a stream as supporting coloured output.
 */
inline void set_colourized(std::ostream& stream) noexcept {
    stream.iword(_detail::colourize_index()) = true;
}

/**
 * @brief Check if a stream supports coloured output.
 */
inline bool is_colourized(std::ostream& stream) noexcept {
    return _detail::is_atty(stream) || static_cast<bool>(stream.iword(_detail::colourize_index()));
}
/**
 * @brief Check if a stream supports coloured output.
 */
#ifdef __cpp_lib_syncstream
bool is_colourized(std::osyncstream& stream) noexcept;
#else
bool is_colourized(basic_osyncstream& stream) noexcept;
#endif

inline void move_up(std::ostream &os, int lines) noexcept { os << "\033[" << lines << "A"; }
inline void move_down(std::ostream &os, int lines) noexcept { os << "\033[" << lines << "B"; }
inline void move_right(std::ostream &os, int cols) noexcept { os << "\033[" << cols << "C"; }
inline void move_left(std::ostream &os, int cols) noexcept { os << "\033[" << cols << "D"; }
inline void erase_line(std::ostream &os) noexcept { os << "\r\033[K";  }

inline void set_stream_colour(std::ostream &os, enum colour col) {
    switch (col) {
    case colour::grey:
        os << termcolour::grey;
        break;
    case colour::red:
        os << termcolour::red;
        break;
    case colour::green:
        os << termcolour::green;
        break;
    case colour::yellow:
        os << termcolour::yellow;
        break;
    case colour::blue:
        os << termcolour::blue;
        break;
    case colour::magenta:
        os << termcolour::magenta;
        break;
    case colour::cyan:
        os << termcolour::cyan;
        break;
    case colour::white:
        os << termcolour::white;
        break;
    default:
        assert(false);
    }
}

inline void set_font_style(std::ostream &os, enum font_style style) {
    switch (style) {
    case font_style::bold:
        os << termcolour::bold;
        break;
    case font_style::dark:
        os << termcolour::dark;
        break;
    case font_style::italic:
        os << termcolour::italic;
        break;
    case font_style::underline:
        os << termcolour::underline;
        break;
    case font_style::blink:
        os << termcolour::blink;
        break;
    case font_style::reverse:
        os << termcolour::reverse;
        break;
    case font_style::concealed:
        os << termcolour::concealed;
        break;
    case font_style::crossed:
        os << termcolour::crossed;
        break;
    default:
        break;
    }
}

}

}
