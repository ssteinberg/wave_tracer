/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <vector>
#include <format>
#include <optional>

#include <concepts>
#include <stdexcept>
#include <cstddef>
#include <cstdio>

#include <wt/math/common.hpp>
#include <wt/math/norm_integers.hpp>

#include <wt/bitmap/load2d.hpp>

#include <png.h>

#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <ImfChannelList.h>

using namespace wt;
using namespace wt::bitmap;


/*
 * OpenEXR
 */

bitmap2d_load_ret_t<f_t> wt::bitmap::load_bitmap2d_exr(const std::filesystem::path &path) {
    // TODO: read 32-bit OpenEXR images

    using namespace Imf;

    RgbaInputFile file(path.string().c_str());
    const auto& dw = file.dataWindow();
    const auto width  = static_cast<std::size_t>(dw.max.x - dw.min.x + 1);
    const auto height = static_cast<std::size_t>(dw.max.y - dw.min.y + 1);
    const pixel_layout_t layout = (file.channels()&WRITE_RGBA) ? pixel_layout_e::RGBA :
                                  (file.channels()&WRITE_RGB)  ? pixel_layout_e::RGB :
                                  pixel_layout_e::L;
    
    const colour_encoding_t ce = colour_encoding_type_e::linear;

    auto out = bitmap2d_t<f_t>::create(width, height, layout);

    std::vector<Rgba> pixels;
    pixels.resize(out.total_pixels());
    file.setFrameBuffer(pixels.data(), 1, width);
    file.readPixels(dw.min.y, dw.max.y);

    for (std::size_t p=0;p<out.total_pixels();++p) {
        out.data()[p*out.components() + 0] = (f_t)pixels[p].r;
        if (out.components()>1) {
            out.data()[p*out.components() + 1] = (f_t)pixels[p].g;
            out.data()[p*out.components() + 2] = (f_t)pixels[p].b;
        }
        if (out.components()>3) 
            out.data()[p*out.components() + 3] = (f_t)pixels[p].a;
    }

    return {
        .bitmap = std::move(out),
        .colour_encoding = ce,
    };
}


/*
 * PNG
 */

std::size_t wt::bitmap::load_bitmap2d_png_query_bitdepth(const std::filesystem::path &file_name) {
    png_byte header[8];

    FILE *fp = fopen(file_name.string().data(), "rb");
    if (!fp) {
        throw std::runtime_error("(load_bitmap2d_png) Could not open file");
    }

    // read the header
    if (!fread(header, 1, 8, fp)) {
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) Not a valid PNG");
    }

    if (png_sig_cmp(header, 0, 8)) {
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) Not a valid PNG");
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) Not a valid PNG");
    }

    // create png info struct
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) Not a valid PNG");
    }

    // create png info struct
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) Not a valid PNG");
    }

    // the code in this if statement gets called if libpng encounters an error
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) libpng error");
    }

    // init png reading
    png_init_io(png_ptr, fp);

    // let libpng know you already read the first 8 bytes
    png_set_sig_bytes(png_ptr, 8);

    // read all the info up to the image data
    png_read_info(png_ptr, info_ptr);

    // variables to pass to get info
    int bit_depth, colour_type;
    png_uint_32 temp_width, height;

    // get info about png
    png_get_IHDR(png_ptr, info_ptr, 
                 &temp_width, &height, &bit_depth, &colour_type,
                 nullptr, nullptr, nullptr);

    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    return bit_depth;
}

template <std::unsigned_integral T>
inline bitmap2d_load_ret_t<T> load_bitmap2d_png(const std::filesystem::path &file_name) {
    // only support unorm8/16
    static_assert(std::is_same_v<T, std::uint8_t> || std::is_same_v<T, std::uint16_t>);

    png_byte header[8];

    FILE *fp = fopen(file_name.string().data(), "rb");
    if (!fp) {
        throw std::runtime_error("(load_bitmap2d_png) Could not open file");
    }

    // read the header
    if (!fread(header, 1, 8, fp)) {
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) Not a valid PNG");
    }

    if (png_sig_cmp(header, 0, 8)) {
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) Not a valid PNG");
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) Not a valid PNG");
    }

    // create png info struct
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) Not a valid PNG");
    }

    // create png info struct
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) Not a valid PNG");
    }

    // the code in this if statement gets called if libpng encounters an error
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) libpng error");
    }

    // init png reading
    png_init_io(png_ptr, fp);

    // let libpng know you already read the first 8 bytes
    png_set_sig_bytes(png_ptr, 8);

    // read all the info up to the image data
    png_read_info(png_ptr, info_ptr);

    // variables to pass to get info
    int bit_depth, colour_type;
    png_uint_32 temp_width, height;

    // get info about png
    png_get_IHDR(png_ptr, info_ptr, 
                 &temp_width, &height, &bit_depth, &colour_type,
                 nullptr, nullptr, nullptr);

    if (bit_depth != 8 && bit_depth != 16) {
        throw std::runtime_error("(load_bitmap2d_png) Unsupported bit depth");
    }

    int components;
    pixel_layout_e layout;
    switch (colour_type) {
    case PNG_COLOR_TYPE_GRAY:
        components = 1;
        layout = pixel_layout_e::L;
        break;
    case PNG_COLOR_TYPE_RGB:
        components = 3;
        layout = pixel_layout_e::RGB;
        break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        components = 4;
        layout = pixel_layout_e::RGBA;
        break;
    default:
        fclose(fp);
        throw std::runtime_error("(load_bitmap2d_png) Unsupported PNG colour type '" + std::format("{:d}",colour_type) + "'");
    }

    if (bit_depth == 16)
        png_set_swap(png_ptr);

    // Update the png info struct.
    png_read_update_info(png_ptr, info_ptr);
    // Row size in bytes.
    auto rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    // Allocate the image_data as a big block
    const size_t width = rowbytes / (components * bit_depth/8);
    
    // row_pointers is for pointing to image_data for reading the png with libpng
    auto** row_pointers = reinterpret_cast<png_byte **>(malloc(height * sizeof(png_byte *)));

    std::optional<bitmap2d_t<T>> image;
    if (bit_depth == 8) {
        auto temp_image = bitmap2d_t<std::uint8_t>::create(width, height, layout);
        // set the individual row_pointers to point at the correct offsets of image_data
        for (unsigned int h=0; h<height; h++)
            row_pointers[h] = &temp_image(0,h,0);

        // read the png into image_data through row_pointers
        png_read_image(png_ptr, row_pointers);

        image = std::move(temp_image).template convert_texels<T>();
    }
    else if (bit_depth == 16) {
        auto temp_image = bitmap2d_t<std::uint16_t>::create(width, height, layout);
        // set the individual row_pointers to point at the correct offsets of image_data
        for (unsigned int h=0; h<height; h++)
            row_pointers[h] = (png_byte*)&temp_image(0,h,0);

        // read the png into image_data through row_pointers
        png_read_image(png_ptr, row_pointers);

        image = std::move(temp_image).template convert_texels<T>();
    }

    fclose(fp);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    free(row_pointers);

    if (!image)
        throw std::runtime_error("(load_bitmap2d_png) Unsupported PNG bit depth");

    // sRGB for uint8 and linear for uint16
    colour_encoding_t ce = bit_depth==16 ? colour_encoding_type_e::linear : colour_encoding_type_e::sRGB;

    return {
        .bitmap = std::move(*image),
        .colour_encoding = ce,
    };
}

bitmap2d_load_ret_t<std::uint8_t> wt::bitmap::load_bitmap2d_png8(const std::filesystem::path &file_name) {
    return load_bitmap2d_png<std::uint8_t>(file_name);
}
bitmap2d_load_ret_t<std::uint16_t> wt::bitmap::load_bitmap2d_png16(const std::filesystem::path &file_name) {
    return load_bitmap2d_png<std::uint16_t>(file_name);
}