/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>

namespace wt::logger {

namespace _detail {

class date_prefix_buf : public std::streambuf {
    std::streambuf* dest;
    bool at_line_start = true;
    std::string name;

protected:
    [[nodiscard]] std::string prefix() const noexcept {
        using namespace std::chrono;
        auto now_ms = floor<milliseconds>(system_clock::now());
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L && defined(__cpp_lib_format)
        return std::format("{:%F %T %z %Z}", zoned_time{ current_zone(), now_ms }) +
               (!name.empty() ? "[" + name + "] " : std::string{}) +
               "\t ――  ";
#else
        // Fallback for systems without full C++20 chrono support
        auto time_t_now = system_clock::to_time_t(system_clock::now());
        auto tm_now = *std::localtime(&time_t_now);
        return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
                          tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                          tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec) +
               (!name.empty() ? "[" + name + "] " : std::string{}) +
               "\t ――  ";
#endif
    }

    // put a single character
    int_type overflow(int_type ch) override {
        if (traits_type::eq_int_type(ch, traits_type::eof()))
            return dest->sputc(ch);

        if (at_line_start) {
            const auto p = prefix();
            dest->sputn(p.data(), static_cast<std::streamsize>(p.size()));
            at_line_start = false;
        }

        if (traits_type::eq_int_type(dest->sputc(ch), traits_type::eof()))
            return traits_type::eof();

        if (ch == '\n')
            at_line_start = true;

        return ch;
    }

    // write a block of characters
    std::streamsize xsputn(const char* s, std::streamsize count) override {
        std::streamsize total_written = 0;
        for (std::streamsize i=0; i<count; ++i) {
            if (traits_type::eq_int_type(overflow(s[i]), traits_type::eof()))
                break;
            ++total_written;
        }
        return total_written;
    }

public:
    date_prefix_buf(std::streambuf* dest, std::string_view name)
        : dest{ dest }, name{ name }
    {}

    int sync() override { return dest->pubsync(); }
};

class date_prefix_ostream : public std::ostream {
    date_prefix_buf buf;

public:
    date_prefix_ostream(std::ostream& dest, std::string_view name)
        : std::ostream{ &buf }, buf{ dest.rdbuf(), name }
    {}
};

}


class file_logger_t {
private:
    std::unique_ptr<_detail::date_prefix_ostream> out_stream, warn_stream, err_stream;
    std::ofstream file_stream;

public:
    file_logger_t(const std::filesystem::path &path) {
        file_stream = std::ofstream(path, std::ofstream::out | std::ofstream::trunc);
        if (file_stream.fail())
            throw std::runtime_error("log file failed to open");

        out_stream = std::make_unique<_detail::date_prefix_ostream>(file_stream, "");
        warn_stream = std::make_unique<_detail::date_prefix_ostream>(file_stream, "Warn");
        err_stream = std::make_unique<_detail::date_prefix_ostream>(file_stream, "ERROR");
    }

    [[nodiscard]] auto& fout() noexcept { return *out_stream; }
    [[nodiscard]] auto& fwarn() noexcept { return *warn_stream; }
    [[nodiscard]] auto& ferr() noexcept { return *err_stream; }

    void flush() { file_stream.flush(); }
};


}
