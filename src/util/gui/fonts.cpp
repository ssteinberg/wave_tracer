/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/util/gui/dependencies.hpp>

#include <wt/util/gui/impl/common.hpp>
#include <wt/util/gui/impl/impl.hpp>


using namespace wt::gui;

void impl_t::load_fonts() {
    const std::string base_font = "Inter.otf";
    const std::string fa1_font  = "fa-regular-400.ttf";
    const std::string fa2_font  = "fa-solid-900.ttf";
    const std::string misc_font = "dejavu-sans.book.ttf";   // fallback font: dejavu has thousands of glyphs
    const std::string mono_font = "Iosevka-wt.bold.ttf";

    const auto fft_path  = ctx.resolve_path(std::filesystem::path{ "data" } / "fonts" / base_font);
    const auto fa1_path  = ctx.resolve_path(std::filesystem::path{ "data" } / "fonts" / fa1_font);
    const auto fa2_path  = ctx.resolve_path(std::filesystem::path{ "data" } / "fonts" / fa2_font);
    const auto misc_path = ctx.resolve_path(std::filesystem::path{ "data" } / "fonts" / misc_font);
    const auto mono_path = ctx.resolve_path(std::filesystem::path{ "data" } / "fonts" / mono_font);

    // base font for common and latin glyph range
    if (fft_path) io->Fonts->AddFontFromFileTTF(fft_path->c_str());
    else {
        logger::cwarn() << "font \'" << base_font << "\' not found.\n";
        io->Fonts->AddFontDefault();
    }

    ImFontConfig config;
    config.MergeMode = true;

    // fontawesome icons
    if (fa1_path && fa2_path) {
        io->Fonts->AddFontFromFileTTF(fa1_path->c_str(), 0, &config);
        io->Fonts->AddFontFromFileTTF(fa2_path->c_str(), 0, &config);
    }
    else
        logger::cwarn() << "font \'" << fa1_font << "\' or \'" << fa2_font << "\' not found.\n";

    // generic font for all misc unicode and symbols
    if (misc_path) {
        io->Fonts->AddFontFromFileTTF(misc_path->c_str(), 0, &config);
    }
    else
        logger::cwarn() << "font \'" << misc_font << "\' not found.\n";

    ImFontAtlasBuildMain(io->Fonts);

    // separate monospaced font
    if (mono_path) {
        this->mono_font = io->Fonts->AddFontFromFileTTF(mono_path->c_str());
    }
    else
        logger::cwarn() << "font \'" << mono_font << "\' not found.\n";
}
