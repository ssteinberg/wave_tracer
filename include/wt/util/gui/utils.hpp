/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <array>
#include <vector>
#include <string>

#include <wt/math/common.hpp>
#include <wt/bitmap/bitmap.hpp>
#include <wt/sensor/film/film_storage.hpp>

#include <wt/spectrum/spectrum.hpp>
#include <wt/spectrum/discrete.hpp>
#include <wt/sensor/response/response.hpp>
#include <wt/util/statistics_collector/stat_histogram.hpp>

#include <wt/util/logger/logger.hpp>

#include "dependencies.hpp"
#include <imgui_internal.h>


namespace wt::gui {

using preview_bitmap_t = sensor::developed_scalar_film_t<2>;
using preview_bitmap_polarimetric_t = sensor::developed_polarimetric_film_t<2>;

struct gl_image_t {
    std::optional<GLuint> handle;
    std::shared_ptr<preview_bitmap_t> image;
    std::uint32_t width, height;

    gl_image_t() noexcept = default;
    gl_image_t(const void* data, std::uint32_t width, std::uint32_t height,
               int channels, std::uint32_t comp_size)
        : width(width), height(height)
    {
        if (channels<=0 || channels>4 || comp_size==0 || comp_size>4 || comp_size==3) {
            assert(false);
            return;
        }

        const auto infmt = channels == 1 ? GL_RED : 
                           channels == 2 ? GL_RG : 
                           channels == 3 ? GL_RGB : 
                                           GL_RGBA;
        const GLenum gl_input_type = comp_size == 4 ? GL_FLOAT : comp_size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;
        int fmt;
        if (comp_size == 4) {
            fmt = channels == 1 ? GL_R32F : 
                  channels == 2 ? GL_RG32F : 
                  channels == 3 ? GL_RGB32F : 
                                  GL_RGBA32F;
        } else if (comp_size == 2) {
            fmt = channels == 1 ? GL_R16 : 
                  channels == 2 ? GL_RG16 : 
                  channels == 3 ? GL_RGB16 : 
                                  GL_RGBA16;
        } else {
            fmt = channels == 1 ? GL_RED : 
                  channels == 2 ? GL_RG : 
                  channels == 3 ? GL_RGB : 
                                  GL_RGBA;
        }

        handle = 0;
        glGenTextures(1, &*handle);
        glBindTexture(GL_TEXTURE_2D, *handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt,
                     width, height, 0, infmt, gl_input_type, data);
    }
    gl_image_t(std::shared_ptr<preview_bitmap_t>&& surface)
        : width(surface->width()), height(surface->height())
    {
        assert(surface->components()>0 && surface->components()<=4);

        const auto infmt = surface->components() == 1 ? GL_RED : 
                           surface->components() == 2 ? GL_RG : 
                           surface->components() == 3 ? GL_RGB : 
                                                        GL_RGBA;
        // internal format, use full 32bit for single channel data (for signal coverage simulations).
        const auto fmt = surface->components() == 1 ? GL_R32F : 
                         surface->components() == 2 ? GL_RG16F : 
                         surface->components() == 3 ? GL_RGB16F : 
                                                      GL_RGBA16F;
        static constexpr GLenum gl_input_type = GL_FLOAT;

        handle = 0;
        glGenTextures(1, &*handle);
        glBindTexture(GL_TEXTURE_2D, *handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt,
                     width, height, 0, infmt, gl_input_type, surface->data());

        this->image = std::move(surface);
    }

    ~gl_image_t() {
        if (handle)
            glDeleteTextures(1, &*handle);
    }

    gl_image_t(gl_image_t&& o) noexcept
        : handle(o.handle), image(std::move(o.image)), width(o.width), height(o.height)
    {
        o.handle = std::nullopt;
    }
    gl_image_t& operator=(gl_image_t&& o) noexcept {
        width  = o.width;
        height = o.height;
        if (handle)
            glDeleteTextures(1, &*handle);
        handle = o.handle;
        o.handle = std::nullopt;
        image = std::move(o.image);
        
        return *this;
    }

    inline explicit operator bool() const noexcept { return !!handle; }
    inline bool operator!() const noexcept { return !handle; }

    inline explicit operator ImTextureID() const noexcept { return (ImTextureID)*handle; }
};

struct gl_images_t {
    std::optional<std::array<GLuint,4>> handles;
    std::shared_ptr<preview_bitmap_polarimetric_t> images;
    std::uint32_t width, height;

    gl_images_t() noexcept = default;
    gl_images_t(std::shared_ptr<preview_bitmap_polarimetric_t>&& surfaces)
        : width(surfaces->front().width()), height(surfaces->front().height())
    {
        const auto comps = surfaces->front().components();
        assert(comps>0 && comps<=4);

        const auto infmt = comps == 1 ? GL_RED : 
                           comps == 2 ? GL_RG : 
                           comps == 3 ? GL_RGB : 
                                        GL_RGBA;
        // internal format, use full 32bit for single channel data (for signal coverage simulations).
        const auto fmt = comps == 1 ? GL_R32F : 
                         comps == 2 ? GL_RG16F : 
                         comps == 3 ? GL_RGB16F : 
                                      GL_RGBA16F;
        static constexpr GLenum gl_input_type = GL_FLOAT;

        handles = std::array<GLuint,4>{ 0,0,0,0 };
        glGenTextures(4, handles->begin());

        for (auto i=0;i<4;++i) {
            glBindTexture(GL_TEXTURE_2D, (*handles)[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, fmt,
                         width, height, 0, infmt, gl_input_type, (*surfaces)[i].data());
        }

        this->images = std::move(surfaces);
    }

    ~gl_images_t() {
        if (handles)
            glDeleteTextures(4, handles->begin());
    }

    gl_images_t(gl_images_t&& o) noexcept
        : handles(o.handles), images(std::move(o.images)), width(o.width), height(o.height)
    {
        o.handles = std::nullopt;
    }
    gl_images_t& operator=(gl_images_t&& o) noexcept {
        width  = o.width;
        height = o.height;
        if (handles)
            glDeleteTextures(4, handles->begin());
        handles = o.handles;
        o.handles = std::nullopt;
        images = std::move(o.images);
        
        return *this;
    }

    inline explicit operator bool() const noexcept { return !!handles; }
    inline bool operator!() const noexcept { return !handles; }

    ImTextureID operator[](int i) const noexcept { return (ImTextureID)(*handles)[i]; }
};


template <typename T>
struct plot_t {
    static constexpr bool has_xticks = true;
    static constexpr bool has_yticks = false;
    static constexpr auto channels = 1;

    std::vector<T> data;
    std::size_t bins;

    static constexpr auto xticks_count = 3;

    std::array<double,xticks_count> xticks;
    std::array<std::string,xticks_count> xtick_labels;
    std::array<const char*,xticks_count> xtick_labels_cstr;

    std::array<std::string,2> imgui_ids_strs;
    std::array<const char*,2> imgui_ids;

    bool new_data = true;

    plot_t(std::string id) noexcept {
        for (int i=0;i<2;++i)
            imgui_ids_strs[i] = std::format("##__plot{}_{}", i, id);
        for (int i=0;i<2;++i)
            imgui_ids[i] = imgui_ids_strs[i].c_str();
    }
    plot_t(std::string id, const stats::stat_histogram_generic_t& histogram) noexcept : plot_t(std::move(id)) {
        const auto& hbins = histogram.get_bins();
        const auto range = histogram.get_range();
        data = std::vector<T>{ hbins.begin(),hbins.end() };
        bins = data.size();

        xticks = { 1,(double)((bins-1)/2+1),(double)bins-2 };
        xtick_labels[0] = std::format("{}", range.min);
        xtick_labels[1] = std::format("{}", (range.min+range.max)/2);
        xtick_labels[2] = std::format("{}", range.max);
        for (auto i=0ul;i<xticks_count;++i)
            xtick_labels_cstr[i] = xtick_labels[i].c_str();
    }
};


template <std::size_t Bins = 512, std::size_t MaxChannels = 3>
struct spectral_plot_t {
    static constexpr auto bins = Bins;
    static constexpr bool has_xticks = true;
    static constexpr bool has_yticks = true;

    static_assert(Bins>0 && MaxChannels>0);

    std::int8_t channels=0;
    std::array<f_t, MaxChannels*Bins> data;

    static constexpr auto xticks_count = 3, yticks_count = 2;

    static constexpr std::array<double,xticks_count> xticks = { 0*(Bins-1),.5*(Bins-1),1*(Bins-1) };
    std::array<std::string,xticks_count> xtick_labels;
    std::array<const char*,xticks_count> xtick_labels_cstr;

    std::array<double,yticks_count> yticks;
    std::array<std::string,yticks_count> ytick_labels;
    std::array<const char*,yticks_count> ytick_labels_cstr;

    std::array<std::string,MaxChannels+1> imgui_ids_strs;
    std::array<const char*,MaxChannels+1> imgui_ids;

    bool new_data = true;

    spectral_plot_t(std::string id) noexcept {
        for (int i=0;i<MaxChannels+1;++i)
            imgui_ids_strs[i] = std::format("##__specplot{}_{}", i, id);
        for (int i=0;i<MaxChannels+1;++i)
            imgui_ids[i] = imgui_ids_strs[i].c_str();
    }

    spectral_plot_t(std::string id, const sensor::response::response_t& response,
           const range_t<wavenumber_t>& krange) noexcept : spectral_plot_t(std::move(id)) {
        const auto& range = range_t<wavelength_t>{
            wavenum_to_wavelen(krange.max), wavenum_to_wavelen(krange.min),
        };

        channels = response.channels();
        for (auto i=0ul;i<Bins;++i) {
            const auto k = wavelen_to_wavenum(m::mix(range,(i+f_t(.5)) / Bins));
            for (std::int8_t c=0;c<channels;++c) {
                const auto v = response.f(c,k);
                data[i+c*Bins] = v;
            }
        }
        compute_spectrum_ticks(range);
    }
    spectral_plot_t(std::string id, const spectrum::discrete_t& spectrum,
           const range_t<wavenumber_t>& krange) noexcept : spectral_plot_t(std::move(id)) {
        const auto& range = range_t<wavelength_t>{
            wavenum_to_wavelen(krange.max), wavenum_to_wavelen(krange.min),
        };

        const auto* dist = dynamic_cast<const discrete_distribution_t<vec2_t>*>(spectrum.distribution());
        if (!dist) {
            assert(false);
            return;
        }

        channels = 1;
        std::fill(data.begin(), data.begin()+channels*Bins, 0);
        for (const auto& v : *dist) {
            const auto wl = wavenum_to_wavelen(v.x/u::mm);
            if (!range.contains(wl)) continue;
            const auto b = m::clamp((std::uint32_t)(u::to_num((wl - range.min) / range.length()) * Bins),
                            0u, (std::uint32_t)Bins - 1);
            data[b] += v.y;
        }

        compute_spectrum_ticks(range);
    }
    spectral_plot_t(std::string id, const spectrum::spectrum_real_t& spectrum,
           const range_t<wavenumber_t>& krange) noexcept : spectral_plot_t(std::move(id)) {
        const auto& range = range_t<wavelength_t>{
            wavenum_to_wavelen(krange.max), wavenum_to_wavelen(krange.min),
        };

        channels = 1;
        for (auto i=0ul;i<Bins;++i) {
            const auto k = wavelen_to_wavenum(m::mix(range,(i+f_t(.5)) / Bins));
            const auto v = spectrum.f(k);
            data[i] = v;
        }
        compute_spectrum_ticks(range);
    }
    spectral_plot_t(std::string id, const spectrum::spectrum_t& spectrum,
           const range_t<wavenumber_t>& krange) noexcept : spectral_plot_t(std::move(id)) {
        const auto& range = range_t<wavelength_t>{
            wavenum_to_wavelen(krange.max), wavenum_to_wavelen(krange.min),
        };

        channels = 2;
        for (auto i=0ul;i<Bins;++i) {
            const auto k = wavelen_to_wavenum(m::mix(range,(i+f_t(.5)) / Bins));
            const auto v = spectrum.value(k);
            data[i+0*Bins] = v.real();
            data[i+1*Bins] = v.imag();
        }
        compute_spectrum_ticks(range);
    }

    inline void compute_spectrum_ticks(const range_t<wavelength_t>& range) noexcept {
        const auto ymax = *std::max_element(data.begin(), data.begin()+channels*Bins);
        yticks = { .0,ymax };
        for (int i=0;i<yticks_count;++i) {
            ytick_labels[i] = std::format("{:e}", yticks[i]);
            ytick_labels_cstr[i] = ytick_labels[i].c_str();
        }

        constexpr auto label = [](auto wl) {
            if (wl>1*u::cm)      return std::format("{::N[.3f]}", wl.in(u::cm));
            else if (wl>1*u::mm) return std::format("{::N[.3f]}", wl.in(u::mm));
            else if (wl>1*u::µm) return std::format("{::N[.3f]}", wl.in(u::µm));
            else                 return std::format("{::N[.3f]}", wl.in(u::nm));
        };

        // labels
        for (int i=0;i<xticks_count;++i) {
            xtick_labels[i] = label(m::mix(range, f_t(i)/f_t(xticks_count-1)));
            xtick_labels_cstr[i] = xtick_labels[i].c_str();
        }
    }
};


template <std::size_t Bins = 512, std::size_t MaxChannels = 3>
struct histogram_t {
    static constexpr auto bins = Bins;
    static constexpr bool has_xticks = true;
    static constexpr bool has_yticks = false;

    static_assert(Bins>0 && MaxChannels>0);

    f_t min=m::inf, max=-m::inf;
    std::int8_t channels;
    std::array<std::uint32_t, MaxChannels*Bins> data;

    static constexpr auto xticks_count = 5;
    static constexpr std::array<double,xticks_count> xticks = { 0*(Bins-1),.25*(Bins-1),.5*(Bins-1),.75*(Bins-1),1*(Bins-1) };
    std::array<std::string,5> xtick_labels;
    std::array<const char*,5> xtick_labels_cstr;

    bool new_data = true;

    // we only use a single histogram; this is an optimization, will break with multiple histograms.
    static std::array<const char*,4> imgui_ids;
    static_assert(MaxChannels<4);

    histogram_t() noexcept = default;

    histogram_t(const preview_bitmap_t& image) noexcept {
        // min and max for all channels
        for (const auto* v=image.data();v!=image.data()+image.total_elements();++v) {
            min = std::min(min,(f_t)*v);
            max = std::max(max,(f_t)*v);
        }

        // cutoff outliers
        const f_t cutoff = .00001f * m::max<f_t>(0,max-min);
        min += cutoff;
        max -= cutoff;

        static const f_t epsilon = 0.001f;
        static const f_t loge = m::log(epsilon);
        static const auto axis_scale = [](f_t val) {
            return val > 0 ? (m::log(val + epsilon) - loge) : -(m::log(-val + epsilon) - loge);
        };
        static const auto inv_axis_scale = [](f_t val) {
            return val > 0 ? (m::exp(val + loge) - epsilon) : -(m::exp(-val + loge) - epsilon);
        };

        const f_t min_log = axis_scale(min);
        const f_t log_range = axis_scale(max) - min_log;
        const f_t recp_log_range = 1 / log_range;

        const auto val_to_bin = [&](f_t val) {
            return m::clamp((std::uint32_t)((axis_scale(val) - min_log) * recp_log_range * Bins),
                            0u, (std::uint32_t)Bins - 1);
        };
        const auto bin_to_val = [&](f_t val) {
            return inv_axis_scale((log_range * val / Bins) + min_log);
        };

        // histogram
        channels = m::min<std::int8_t>(MaxChannels,image.components());
        std::fill(data.begin(), data.begin()+channels*Bins, 0);
        for (std::size_t p=0;p<image.total_pixels();++p) {
            for (std::int8_t c=0;c<channels;++c) {
                const auto x = image.data()[p*channels + c];
                if (x<min || x>max) continue;
                const auto bin = val_to_bin(x);
                ++data[c*Bins + bin];
            }
        }

        // labels
        for (int i=0;i<xticks_count;++i) {
            xtick_labels[i] = std::format("{:.2e}", bin_to_val(m::mix<f_t>(0,Bins-1,f_t(i)/f_t(xticks_count-1))));
            xtick_labels_cstr[i] = xtick_labels[i].c_str();
        }
    }
};

template <std::size_t Bins, std::size_t MaxChannels>
std::array<const char*,4> histogram_t<Bins,MaxChannels>::imgui_ids = {
    "##__histogram_0","##__histogram_1","##__histogram_2","##__histogram_3"
};


enum class gui_state_t : std::uint8_t {
    loading,
    rendering,
    idle
};

enum log_type_e : std::int8_t {
    cout = 0, cwarn = 1, cerr = 2,
    log_type_count = 3
};

enum preview_mode_e : std::int8_t {
    linear = 0, gamma = 1, db = 2, fc = 3,
    preview_mode_count = 4
};

struct logbox_ctx_t {
    bool sout_enabled[log_type_e::log_type_count] = { true,true,true };
};

}
