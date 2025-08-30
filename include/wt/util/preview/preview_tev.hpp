/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <string>
#include <memory>

#include <wt/math/common.hpp>
#include <wt/bitmap/bitmap.hpp>

#include "preview_interface.hpp"

namespace wt {

class preview_tev_t final : public preview_interface_t {
private:
    std::shared_ptr<void> ptr;

public:
    preview_tev_t(const std::string& host, const std::int32_t port);

    /**
     * @brief Updates the preview image. Can be called from any thread.
     */
    void update(const std::string& preview_id, sensor::developed_scalar_film_t<2>&& surface,
                const f_t spp_completed,
                const sensor::response::tonemap_t* tonemap = nullptr) const override;

    /**
     * @brief Updates the preview image (polarimetric input). Can be called from any thread.
     */
    void update(const std::string& preview_id, sensor::developed_polarimetric_film_t<2>&& surface,
                const f_t spp_completed,
                const sensor::response::tonemap_t* tonemap = nullptr) const override {
        // ignore polarimetric output
        update(preview_id, std::move(surface[0]), spp_completed, tonemap);
    }

    [[nodiscard]] bool polarimetric_preview() const noexcept override { return false; }
    [[nodiscard]] bool available() const noexcept override { return true; }
};

}
