/*
 *
 * wave tracer
 * Copyright  Shlomi Steinberg
 * Authors:  Umut Emre, Shlomi Steinberg
 *
 * LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
 *
 */

#pragma once

#include <wt/util/concepts.hpp>
#include <wt/util/logger/logger.hpp>

#include "stat_collector.hpp"

#include <format>

namespace wt::stats {

/**
 * @brief Counter to track a scalar quantity. 
 *        NOT thread-safe.
 */
template <Scalar T>
class stat_counter_t final : public stat_collector_t {
    T counter = {};

    std::ostream& output(std::ostream& os) const override {
        if (this->flags.ignore_when_empty && is_empty())
            return os;

        constexpr std::size_t maxlabelw = name_label_maxw-print_indent-1;
        auto label = name.length()<=maxlabelw ? name : name.substr(0, maxlabelw);
        if (label.length()<maxlabelw)
            label += std::string(maxlabelw-label.length(), ' ');

        std::string str;
        const auto counter_and_suffix = stat_value_with_suffix(counter);
        if (counter_and_suffix)
            str = std::format("{:>9.4f}{}", counter_and_suffix->first, counter_and_suffix->second);
        else
            str = std::format("{:>10}", counter);
        
        using namespace logger::termcolour;

        os << std::string(print_indent, ' ') 
           << reset << bright_white
           << label
           << reset << '\t' << yellow << bold
           << str
           << reset;
        os << '\n';

        return os;
    }
    std::ostream& output(std::ofstream& fs) const override {
        if (this->flags.ignore_when_empty && counter == T{})
            return fs;

        return fs << name << ", , " << counter << std::endl;
    }

    stat_collector_t& operator+=(const stat_collector_t& rhs) override {
        const auto& rhs_counter = dynamic_cast<const stat_counter_t&>(rhs);
        counter += rhs_counter.counter;
        return *this;
    }

    [[nodiscard]] std::unique_ptr<stat_collector_t> zero() const override {
        return std::make_unique<stat_counter_t<T>>(name, flags);  
    }

public:
    explicit stat_counter_t(std::string name, 
                            stat_collector_flags_t flags = {}) noexcept
        : stat_collector_t(std::move(name), flags)
    {}

    [[nodiscard]] bool is_empty() const noexcept override {
        return counter==T{};
    }

    void operator++() noexcept { ++counter; }
    void operator--() noexcept { --counter; }
    stat_counter_t& operator+=(const T& val) noexcept {
        counter += val;
        return *this;
    }

    stat_counter_t& operator-=(const T& val) noexcept {
        counter -= val;
        return *this;
    }

    void set(const T& val) noexcept {
        counter = val;
    }
};

}  // namespace wt
