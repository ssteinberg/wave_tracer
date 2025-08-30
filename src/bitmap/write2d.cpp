/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <cstdio>

#include <concepts>
#include <stdexcept>
#include <cstddef>

#include <wt/math/common.hpp>
#include <wt/math/norm_integers.hpp>
#include <wt/util/format/utils.hpp>

#include <wt/bitmap/write2d.hpp>

#include <wt/util/logger/logger.hpp>

#include <png.h>

#define throw(...)
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfOutputFile.h>
#include <ImfFrameBuffer.h>
#include <ImfStringAttribute.h>
#undef throw

using namespace wt;
using namespace wt::bitmap;


/*
 * OpenEXR
 */

void wt::bitmap::write_bitmap2d_exr(const std::filesystem::path &path,
                                    const bitmap2d_t<float> &s, 
                                    const std::vector<std::string>& channel_names,
                                    const std::map<std::string,std::string>& attributes) {
    using namespace Imf;

    const auto components = s.components();
    const auto channel_name = [&](std::size_t c) -> std::string {
        if (c<channel_names.size()) return channel_names[c];
        if (components==1) return "L";
        static constexpr std::array<char, 4> RGBA = { 'R', 'G', 'B', 'A' };
        if (c >= 0 && c < 4) return std::string{ 1,RGBA[c] };
        return std::string{ "X" };
    };

    auto header = Header((int)s.width(), (int)s.height());
    for (const auto& a : attributes) {
        header.insert(a.first, StringAttribute{ a.second });
    }

    FrameBuffer frameBuffer;
    for (std::size_t c=0;c<components;++c) {
        const std::string name = channel_name(c);

        header.channels().insert(name, Channel(FLOAT));        
        frameBuffer.insert(
            name,
            Slice(FLOAT,
                  (char*)&s.data()[c],
                  sizeof(float) * components,
                  sizeof(float) * components * s.width())
        );
    }

    auto file = OutputFile(path.string().c_str(), header);
    file.setFrameBuffer(frameBuffer);
    file.writePixels((int)s.height());
}


/*
 * PNG
 */

template <std::integral T>
inline void write_bitmap2d_png_impl(const std::filesystem::path &path, const bitmap2d_t<T> &s) {
    if (sizeof(T)!=1 && sizeof(T)!=2)
        throw std::runtime_error("(write_bitmap2d_png) Only 8-bit or 16-bit bitmaps are supported");

    const auto& file_name = path.string();

    int components = s.components(), width = static_cast<int>(s.width()), height = static_cast<int>(s.height());

    FILE *fp = fopen(file_name.c_str(), "wb");
    if (!fp) {
        logger::cerr() << file_name << " can't be opened for writing" << '\n';
        throw std::runtime_error("(write_bitmap2d_png) PNG save error");
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        logger::cerr() << file_name << " png_create_write_struct failed" << '\n';
        fclose(fp);
        throw std::runtime_error("(write_bitmap2d_png) PNG save error");
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        logger::cerr() << file_name << " png_create_info_struct failed" << '\n';
        fclose(fp);
        throw std::runtime_error("(write_bitmap2d_png) PNG save error");
    }

    if (setjmp(png_jmpbuf(png))) {
        logger::cerr() << file_name << " png_jmpbuf failed" << '\n';
        fclose(fp);
        throw std::runtime_error("(write_bitmap2d_png) PNG save error");
    }

    png_byte ** const row_pointers = reinterpret_cast<png_byte **>(malloc(std::size_t(height) * sizeof(png_byte *)));
    if (row_pointers == nullptr) {
        logger::cerr() << file_name << " could not allocate memory for PNG row pointers" << '\n';
        fclose(fp);
        throw std::runtime_error("(write_bitmap2d_png) PNG save error");
    }

    std::optional<bitmap2d_t<T>> bms;
    if constexpr (sizeof(T)>1) {
        // flip byte order
        const auto comps = s.components();
        bms = s.template convert<T>(s.pixel_layout(), [&](const auto* src, auto* dst) {
            for (int c=0;c<comps;++c) {
                dst[c] = format::byteswap(src[c]);
            }
        });

        // set the individual row_pointers to point at the correct offsets of image_data
        for (int i = 0; i < height; i++)
            row_pointers[i] = (png_byte*)&(*bms)(0,(std::uint32_t)i,0);
    }
    else {
        // set the individual row_pointers to point at the correct offsets of image_data
        for (int i = 0; i < height; i++)
            row_pointers[i] = (png_byte*)&s(0,(std::uint32_t)i,0);
    }

    png_init_io(png, fp);

    int colour_type;
    switch (components) {
    case 1: colour_type = PNG_COLOR_TYPE_GRAY; break;
    case 3: colour_type = PNG_COLOR_TYPE_RGB; break;
    case 4: colour_type = PNG_COLOR_TYPE_RGBA; break;
    default: {
        logger::cerr() << "Invalid component count" << '\n';
        throw std::runtime_error("(write_bitmap2d_png) PNG save error");
    }
    }
    
    png_set_IHDR(png,
        info,
        (png_uint_32)width, (png_uint_32)height,
        8*sizeof(T),
        colour_type,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    png_write_image(png, row_pointers);
    png_write_end(png, nullptr);

    free(row_pointers);
    fclose(fp);
}

void wt::bitmap::write_bitmap2d_png(const std::filesystem::path &path,
                                    const bitmap2d_t<std::uint8_t> &s) {
    return write_bitmap2d_png_impl(path, s);
}
void wt::bitmap::write_bitmap2d_png(const std::filesystem::path &path,
                                    const bitmap2d_t<std::uint16_t> &s) {
    return write_bitmap2d_png_impl(path, s);
}
