/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <stdexcept>

#include <string>
#include <filesystem>

#include <wt/util/logger/logger.hpp>
#include <wt/util/concepts.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <wt/util/font_renderer/font_renderer.hpp>

#include <wt/math/glm.hpp>
#include <glm/gtx/color_space.hpp>

using namespace wt;


namespace font_renderer {

struct impl_t {
    FT_Library ft2_library;
    FT_Face ft2_face;

    impl_t(const std::filesystem::path& font_path) {
        auto error = FT_Init_FreeType(&ft2_library);
        if (error) 
            throw std::runtime_error("Freetype2 error: " + std::string{ FT_Error_String(error) });

        error = FT_New_Face(ft2_library, font_path.c_str(), 0, &ft2_face);
        if (error) 
            throw std::runtime_error("Freetype2 error: " + std::string{ FT_Error_String(error) });
    }

    ~impl_t() {
        FT_Done_Face(ft2_face);
        FT_Done_FreeType(ft2_library);
    }
};

}

font_renderer_t::font_renderer_t(const wt_context_t& ctx, const std::string& font) {
    const auto path = std::filesystem::path{ "data" } / "fonts" / font;
    const auto resolved_path = ctx.resolve_path(path);
    if (!resolved_path)
        throw std::runtime_error("(font_renderer) font '" + path.string() + "' not found.");
            
    auto pimpl = new font_renderer::impl_t(*resolved_path);
    this->ptr = std::shared_ptr<font_renderer::impl_t>(pimpl);
}

template <FloatingPoint Fp>
vec2_t renderer(font_renderer::impl_t* pimpl,
                const std::string& text, 
                bitmap::bitmap2d_t<Fp>& target,
                vec2u32_t position,
                const f_t text_size_px,
                const font_renderer_t::anchor_t anchor,
                const std::optional<vec4_t>& colour) {
    const auto error = FT_Set_Char_Size(pimpl->ft2_face, std::size_t(text_size_px*64), 0, 0, 0);
    if (error) 
        throw std::runtime_error("Freetype2 error: " + std::string{ FT_Error_String(error) });

    if (target.components()!=3 && target.components()!=1) {
        logger::cerr() << "(font_renderer) Only 1-component (L) and 3-component (RGB) target bitmaps are supported." << '\n';
        return {};
    }

    // calculate size
    const auto slot = pimpl->ft2_face->glyph;
    FT_Vector pen = { 0,0 };
    for (const auto& ch : text) {
        FT_Set_Transform(pimpl->ft2_face, nullptr, &pen);
        const auto error = FT_Load_Char(pimpl->ft2_face, ch, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING);
        if (error)
            continue;
        pen.x += slot->advance.x;
        pen.y += slot->advance.y;
    }

    const auto size = vec2_t{ pen.x/f_t(64),(pen.y+pimpl->ft2_face->size->metrics.height)/f_t(64) };

    switch (anchor) {
    case font_renderer_t::anchor_t::top_left:
        position += vec2u32_t{ 0,size.y };
        break;
    case font_renderer_t::anchor_t::top:
        position += vec2u32_t{ -size.x/2,size.y };
        break;
    case font_renderer_t::anchor_t::top_right:
        position += vec2u32_t{ -size.x,size.y };
        break;
    case font_renderer_t::anchor_t::right:
        position += vec2u32_t{ -size.x,size.y/2 };
        break;
    case font_renderer_t::anchor_t::bottom_right:
        position += vec2u32_t{ -size.x,0 };
        break;
    case font_renderer_t::anchor_t::bottom:
        position += vec2u32_t{ -size.x/2,0 };
        break;
    case font_renderer_t::anchor_t::bottom_left:
        break;
    case font_renderer_t::anchor_t::left:
        position += vec2u32_t{ 0,size.y/2 };
        break;
    }

    // glyph colours for dark and bright regions
    const vec4_t colour_dark   = colour.value_or(vec4_t{ 0,0,0,.75 });
    const vec4_t colour_bright = colour.value_or(vec4_t{ 1,1,1,.75 });

    // render
    pen = { 0,0 };
    for (const auto& ch : text) {
        FT_Set_Transform(pimpl->ft2_face, nullptr, &pen);
        const auto error = FT_Load_Char(pimpl->ft2_face, ch, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
        if (error)
            continue;

        for (int y=0; y<slot->bitmap.rows;  ++y)
        for (int x=0; x<slot->bitmap.width; ++x) {
            const int ix = position.x + slot->bitmap_left+x, 
                      iy = position.y - slot->bitmap_top+y;
            if (ix>=target.dimensions().x || iy>=target.dimensions().y)
                break;

            // glyph
            const auto g = slot->bitmap.buffer[x+slot->bitmap.width*y];

            if (target.components()==1) {
                f_t& p = *reinterpret_cast<f_t*>(&target(ix,iy,0));

                const auto c = p<.5 ? colour_bright : colour_dark;
                p = m::mix(p, glm::luminosity(vec3_t{ c }), f_t(g)/255);
            }
            else if (target.components()==3) {
                vec3_t& p = *reinterpret_cast<vec3_t*>(&target(ix,iy,0));

                const auto c = glm::luminosity(p)<.5 ? colour_bright : colour_dark;
                const auto a = c.w*f_t(g)/255;
                p = m::mix(p, vec3_t{ c }, a);
            }
        }

        pen.x += slot->advance.x;
        pen.y += slot->advance.y;
    }

    return size;
}

vec2_t font_renderer_t::render(
        const std::string& text, 
        bitmap::bitmap2d_t<float>& target,
        vec2u32_t position,
        const f_t text_size_px,
        const anchor_t anchor,
        const std::optional<vec4_t>& colour) const {
    const auto pimpl = static_cast<font_renderer::impl_t*>(ptr.get());
    return renderer(pimpl, text, target, position, text_size_px, anchor, colour);
}

vec2_t font_renderer_t::render(
        const std::string& text, 
        bitmap::bitmap2d_t<double>& target,
        vec2u32_t position,
        const f_t text_size_px,
        const anchor_t anchor,
        const std::optional<vec4_t>& colour) const {
    const auto pimpl = static_cast<font_renderer::impl_t*>(ptr.get());
    return renderer(pimpl, text, target, position, text_size_px, anchor, colour);
}
