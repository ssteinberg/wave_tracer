// ImGuiTexInspect, a texture inspector widget for dear imgui
// adapted from https://github.com/andyborrell/imgui_tex_inspect/

#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_tex_inspect.h"

#include <wt/bitmap/bitmap.hpp>
#include <wt/sensor/response/RGB.hpp>

#include <memory>

namespace ImGuiTexInspect
{
//-------------------------------------------------------------------------
// [SECTION] UTILITIES
//-------------------------------------------------------------------------
// Returns true if a flag is set
template <typename TSet, typename TFlag>
static inline bool HasFlag(TSet set, TFlag flag)
{
    return (set & flag) == flag;
}

// Set flag or flags in set
template <typename TSet, typename TFlag>
static inline void SetFlag(TSet &set, TFlag flags)
{
    set = static_cast<TSet>(set | flags);
}

// Clear flag or flags in set
template <typename TSet, typename TFlag>
static inline void ClearFlag(TSet &set, TFlag flag)
{
    set = static_cast<TSet>(set & ~flag);
}

// Proper modulus operator, as opposed to remainder as calculated by %
template <typename T>
static inline T Modulus(T a, T b)
{
    return a - b * ImFloor(a / b);
}

static inline float Round(float f)
{
    return ImFloor(f + 0.5f);
}

static inline ImVec2 Abs(ImVec2 v)
{
    return ImVec2(ImAbs(v.x), ImAbs(v.y));
}

//-------------------------------------------------------------------------
// [SECTION] STRUCTURES
//-------------------------------------------------------------------------

enum Mode_e : int {
    linear = 0,
    gamma = 1,
    db = 2,
    fc = 3,
};
enum PolMode_e : int {
    pol_filter = 0,
    DOP = 1,
    DOLP = 2,
    DOCP = 3,
    LPDir = 4,
    Error = 5
};

struct ShaderOptions
{
    ImVec4 BackgroundColor                = {0,0,0,0}; // Color used for alpha blending
    float  PremultiplyAlpha               = 0;         // If 1 then color will be multiplied by alpha in shader, before blend stage 
    float  DisableFinalAlpha              = 0;         // If 1 then fragment shader will always output alpha = 1

    bool   ForceNearestSampling           = false;     // If true fragment shader will always sample from texel centers

    ImVec2 GridWidth                      = {0,0};     // Width in UV coords of grid line
    ImVec4 GridColor                      = {0,0,0,0};

    Mode_e Mode = linear;
    ImVec4 ModeData = {};
    int colourmap=0;

    int is_polarimetric = 0;
    PolMode_e PolMode = pol_filter;
    ImVec4 stokes_filter = { 1,0,0,0 };

    void   ResetColorTransform();
    ShaderOptions();
};

inline auto dot(const ImVec4& a, const ImVec4& b) noexcept {
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

struct Inspector {
    ImGuiID ID;
    bool Initialized = false;

    const wt::sensor::response::RGB_t* rgb_response_function = nullptr;
    wt::mat3_t XYZ_to_RGB, RGB_to_XYZ;

    // Texture
    std::array<ImTextureID,4> Textures = { ImTextureID{},ImTextureID{},ImTextureID{},ImTextureID{} };

    // View State
    bool IsDragging = false;            // Is user currently dragging to pan view
    ImVec2 PanPos = {0.5f, 0.5f};       // The UV value at the center of the current view
    ImVec2 Scale = {1, 1};              // 1 pixel is 1 texel

    ImVec2 PanelTopLeftPixel = {0, 0};  // Top left of view in ImGui pixel coordinates
    ImVec2 PanelSize = {0, 0};          // Size of area allocated to drawing the image in pixels.

    ImVec2 ViewTopLeftPixel = {0, 0};   // Position in ImGui pixel coordinates
    ImVec2 ViewSize = {0, 0};           // Rendered size of current image. This could be smaller than panel size if user has zoomed out.
    ImVec2 ViewSizeUV = {0, 0};         // Visible region of the texture in UV coordinates

    std::shared_ptr<wt::bitmap::bitmap2d_t<float>> image;
    std::shared_ptr<std::array<wt::bitmap::bitmap2d_t<float>,4>> images;
    bool Stokes = false;
    Transform2D TexelsToPixels;
    Transform2D PixelsToTexels;

    // Configuration
    InspectorFlags Flags = 0;

    // Background mode
    InspectorAlphaMode AlphaMode = InspectorAlphaMode_ImGui;
    ImVec4 CustomBackgroundColor = {0, 0, 0, 1};

    // Scaling limits
    ImVec2 ScaleMin = {0.02f, 0.02f};
    ImVec2 ScaleMax = {500, 500};

    // Grid
    float MinimumGridSize = 4; // Don't draw the grid if lines would be closer than MinimumGridSize pixels

    // Annotations
    ImU32 MaxAnnotatedTexels = 0;

    // Color transformation
    ShaderOptions ActiveShaderOptions;
    ShaderOptions CachedShaderOptions;


    [[nodiscard]] inline auto texture_size() const {
        if (Stokes)
            return images ?
                ImVec2{ (float)(*images)[0].dimensions().x, (float)(*images)[0].dimensions().y } :
                ImVec2{ 1,1 };
        return image ?
            ImVec2{ (float)image->dimensions().x, (float)image->dimensions().y } :
            ImVec2{ 1,1 };
    }

    [[nodiscard]] inline bool is_rgba_image() const {
        if (Stokes)
            return images ? (*images)[0].components()==4 : false;
        return image ? image->components()==4 : false;
    }
    [[nodiscard]] inline bool is_rgb_image() const {
        if (Stokes)
            return images ? (*images)[0].components()>=3 : false;
        return image ? image->components()>=3 : false;
    }

    ImVec4 get_texel(int x, int y) {
        if (!images && !image)
            return ImVec4{ 0,0,0,0 };

        ImVec4 ret = { 0,0,0,1 };

        if (!Stokes) {
            ret.x = (*image)(x,y,0);
            if (image->components()>=3) {
                ret.y = (*image)(x,y,1);
                ret.z = (*image)(x,y,2);
            }
            if (image->components()>=4)
                ret.w = (*image)(x,y,3);
        } else {
            ImVec4 S[4];
            for (int i=0;i<4;++i) {
                S[i] = ImVec4{ (float)(*images)[i](x,y,0),0,0,1 };
                if ((*images)[0].components()>=3) {
                    S[i].y = (*images)[i](x,y,1);
                    S[i].z = (*images)[i](x,y,2);
                }
                if ((*images)[0].components()>=4)
                    S[i].w = (*images)[i](x,y,3);
            }

            ret.x = dot(ImVec4{ S[0].x,S[1].x,S[2].x,S[3].x }, ActiveShaderOptions.stokes_filter);
            ret.y = dot(ImVec4{ S[0].y,S[1].y,S[2].y,S[3].y }, ActiveShaderOptions.stokes_filter);
            ret.z = dot(ImVec4{ S[0].z,S[1].z,S[2].z,S[3].z }, ActiveShaderOptions.stokes_filter);
            ret.w = 1;
        }

        return ret;
    }
};

//-------------------------------------------------------------------------
// [SECTION] INTERNAL FUNCTIONS
//-------------------------------------------------------------------------

Inspector *GetByKey(const Context *ctx, ImGuiID key);
Inspector *GetOrAddByKey(Context *ctx, ImGuiID key);

void SetPanPos(Inspector *inspector, ImVec2 pos);
void SetScale(Inspector *inspector, ImVec2 scale);
void SetScale(Inspector *inspector, float scaleY);
void RoundPanPos(Inspector *inspector);

/* GetTexelsToPixels 
 * Calculate a transform to convert from texel coordinates to screen pixel coordinates
 * */
Transform2D GetTexelsToPixels(ImVec2 screenTopLeft, ImVec2 screenViewSize, ImVec2 uvTopLeft, ImVec2 uvViewSize, ImVec2 textureSize);

/* PushDisabled & PopDisabled
 * Push and Pop and ImGui styles that disable and "grey out" ImGui elements
 * by making them non interactive and transparent*/
void PushDisabled();
void PopDisabled();

//-------------------------------------------------------------------------
// [SECTION] BACKEND FUNCTIONS
//-------------------------------------------------------------------------
void BackEnd_SetShader(const ImDrawList *drawList, const ImDrawCmd *cmd, const Inspector *inspector);

} // namespace ImGuiTexInspect
