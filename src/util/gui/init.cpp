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
#include <wt/util/gui/imgui/style.hpp>
#include <wt/util/gui/imgui/utils.hpp>

#include <wt/bitmap/load2d.hpp>


using namespace wt::gui;

void impl_t::init(const wt_context_t& ctx) {
    const auto& title = this->gui_title();

    if (!SDL_Init(SDL_INIT_VIDEO))
        throw std::runtime_error(std::string{ "Error: SDL_Init(): " } + SDL_GetError());

    // do not disable compositor on linux
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    // allow screensaver
    SDL_EnableScreenSaver();

    const char* glsl_version = "#version 420";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    auto window_size = initial_window_size;
    int displayCount;
    if (const auto* displays = SDL_GetDisplays(&displayCount); displays) {
        SDL_Rect display_res;
        if (SDL_GetDisplayBounds(displays[0], &display_res)) {
            if (display_res.h < 1600)
                window_size = initial_window_size_small;
        }
    }

    const SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    window = SDL_CreateWindow(title.data(), window_size.x, window_size.y, window_flags);
    if (window == nullptr)
        throw std::runtime_error(std::string{ "Error: SDL_CreateWindow(): " } + SDL_GetError());

    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
        throw std::runtime_error(std::string{ "Error: SDL_GL_CreateContext(): " } + SDL_GetError());

    if (glewInit() != GLEW_OK)
        throw std::runtime_error("glewInit() failed");

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // icon
    const auto path = ctx.resolve_path("icon.png");
    if (path) {
        const auto icn = bitmap::load_bitmap2d_png8(*path).bitmap;
        this->icon = gl_image_t{ icn.data(), icn.width(), icn.height(), icn.components(), (uint32_t)icn.component_bytes() };

        // window icon
        const auto icnargb = icn.convert<std::uint8_t>(bitmap::pixel_layout_e::RGBA,
            [](const std::uint8_t* src, std::uint8_t *dst){
                dst[0] = src[3];
                dst[1] = src[0];
                dst[2] = src[1];
                dst[3] = src[2];
            });
        if (icn.pixel_layout()==bitmap::pixel_layout_e::RGBA) {
            const auto surface = SDL_CreateSurfaceFrom(
                icnargb.width(), icnargb.height(),
                SDL_PIXELFORMAT_ARGB8888, (void*)icnargb.data(),
                icnargb.width()*4);
            SDL_SetWindowIcon(window, surface);
            SDL_DestroySurface(surface);
        }
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    io = &ImGui::GetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

#ifdef RELEASE
    ImGui::GetCurrentContext()->DebugLogFlags = 0;
#endif

    // ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
    gui::set_imgui_style();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGuiTexInspect::ImplOpenGL3_Init(glsl_version);
    ImGuiTexInspect::Init();
    ImGuiTexInspect::CreateContext();

    // set minimum window size after GL context creation
    SDL_SetWindowMinimumSize(window, minimum_window_size.x, minimum_window_size.y);

    // print renderer info
    const auto renderer = std::string{ (const char*)glGetString(GL_RENDERER) };
    logger::cout(verbosity_e::info) << "(gui) initialized opengl context, render: \"" << renderer << "\".\n";

    // create colourmap legend bars
    for (int idx=0;idx<colourmap_legend_bars.size();++idx) {
        const auto map = ImGuiTexInspect::colourmaps[idx];

        auto cmp = bitmap::bitmap2d_t<std::uint32_t>::create(
            previewer_colourmap_bar_legend_size.x, previewer_colourmap_bar_legend_size.y, bitmap::pixel_layout_e::L);
        for_range(vec2u32_t{ 0 }, previewer_colourmap_bar_legend_size, [&](auto idx) {
            const auto v = tinycolormap::GetColor(idx.x / (float)(previewer_colourmap_bar_legend_size.x-1), map);
            cmp(idx,0) = glm::packUnorm4x8(vec4<float>{ v[0],v[1],v[2],1});
        });

        colourmap_legend_bars[idx] = gl_image_t{
            cmp.data(),cmp.width(),cmp.height(), 4, 1
        };
    }
}

void impl_t::new_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void impl_t::render() {
    const auto& cc = window_clear_colour;

    ImGui::Render();
    glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
    glClearColor(cc.x * cc.w, cc.y * cc.w, cc.z * cc.w, cc.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

void impl_t::deinit() {
    // cleanup
    ImGuiTexInspect::ImplOpenGl3_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
