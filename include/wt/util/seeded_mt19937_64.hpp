/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <random>
#include <thread>
#include <chrono>

namespace wt {

class seeded_mt19937_64 {
private:
    std::mt19937_64 dev;
    
public:
    seeded_mt19937_64() {
        auto s = seed();
        dev = std::mt19937_64{ s };
    }

    auto &engine() noexcept { return dev; }

private:
    static std::seed_seq seed() noexcept {
        using namespace std::chrono;

        std::random_device r;

        const auto tid  = (std::uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id());
        const auto secs = (std::uint32_t)duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        
        // seed with random numbers, thread id and time
        return std::seed_seq{ 
            r() ^ tid,
            r() ^ secs,
            r() ^ tid,
            r() ^ secs,
            r() ^ tid,
            r() ^ secs,
            r() ^ tid,
            r() ^ secs
        };
    }
};

}
