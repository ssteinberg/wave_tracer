/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <functional>
#include "render_results.hpp"

namespace wt::scene::interrupts {

class interrupt_t {
public:
    virtual ~interrupt_t() = 0;
};
inline interrupt_t::~interrupt_t() {}

/**
 * @brief Terminates the rendering.
 */
class terminate_t final : public interrupt_t {
public:
    ~terminate_t() noexcept = default;
};

/**
 * @brief Pauses the rendering. Does nothing if already paused or pausing.
 */
class pause_t final : public interrupt_t {
public:
    ~pause_t() noexcept = default;
};

/**
 * @brief Resumes the rendering. Does nothing if not paused or pausing.
 */
class resume_t final : public interrupt_t {
public:
    ~resume_t() noexcept = default;
};

/**
 * @brief Queues capturing an intermediate rendered result.
 *        Produces a complete render with all its blocks complete.
 */
class capture_intermediate_t final : public interrupt_t {
public:
    /**
     * @brief Callback for developed result .
     */
    using callback_t = std::function<void(render_result_t)>;

private:
    callback_t callback;

public:
    capture_intermediate_t(callback_t callback) : callback(std::move(callback)) {}
    ~capture_intermediate_t() noexcept = default;

    inline void operator()(render_result_t results) const {
        callback(std::move(results));
    }
};

}
