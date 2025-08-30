/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <wt/math/common.hpp>
#include <wt/bitmap/bitmap.hpp>
#include <wt/sensor/film/defs.hpp>

namespace wt {

namespace sensor::response {
class tonemap_t;
}

constexpr inline std::chrono::milliseconds default_preview_update_interval() noexcept {
    using namespace std::chrono_literals;
    return 2000 * (1ms);
}

class preview_interface_t {
public:
    virtual ~preview_interface_t() noexcept = default;

    /**
     * @brief Indicates a desired minimal interval from clients calling update().
     */
    [[nodiscard]] virtual std::chrono::milliseconds preview_update_interval() const noexcept {
        return default_preview_update_interval();
    }
    /**
     * @brief Indicates a desired rate limiting factor from clients calling update().
     */
    [[nodiscard]] virtual unsigned preview_update_rate_limit_factor() const noexcept {
        return 4;
    }

    /**
     * @brief Updates the preview image. Can be called from any thread.
     */
    virtual void update(const std::string& preview_id, sensor::developed_scalar_film_t<2>&& surface,
                        const f_t spp_completed,
                        const sensor::response::tonemap_t* tonemap = nullptr) const = 0;

    /**
     * @brief Updates the preview image (polarimetric input). Can be called from any thread.
     */
    virtual void update(const std::string& preview_id, sensor::developed_polarimetric_film_t<2>&& surface,
                        const f_t spp_completed,
                        const sensor::response::tonemap_t* tonemap = nullptr) const = 0;

    /**
     * @brief Returns TRUE if the preview is able to process polarimetric inputs.
     *        Must be thread safe.
     */
    [[nodiscard]] virtual bool polarimetric_preview() const noexcept = 0;
    /**
     * @brief Returns TRUE if the preview is available to process new input.
     *        Must be thread safe.
     */
    [[nodiscard]] virtual bool available() const noexcept = 0;
};

}
