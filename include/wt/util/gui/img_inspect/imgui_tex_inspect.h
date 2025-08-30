// ImGuiTexInspect, a texture inspector widget for dear imgui
// adapted from https://github.com/andyborrell/imgui_tex_inspect/

#pragma once
#include "imgui.h"

#include <wt/bitmap/bitmap.hpp>
#include <wt/sensor/response/RGB.hpp>

#include <tinycolormap.hpp>


namespace wt::gui {

struct gl_image_t;
struct gl_images_t;

}

namespace ImGuiTexInspect {
struct Context;
struct Transform2D;
//-------------------------------------------------------------------------
// [SECTION] INIT & SHUTDOWN
//-------------------------------------------------------------------------
void Init();
void Shutdown();

Context *CreateContext();
void DestroyContext(Context *);
void SetCurrentContext(Context *);

//-------------------------------------------------------------------------
// [SECTION] BASIC USAGE
//-------------------------------------------------------------------------

enum InspectorAlphaMode {
    InspectorAlphaMode_ImGui,      // Alpha is transparency so you see the ImGui panel background behind image
    InspectorAlphaMode_Black,      // Alpha is used to blend over a black background
    InspectorAlphaMode_White,      // Alpha is used to blend over a white background
    InspectorAlphaMode_CustomColor // Alpha is used to blend over a custom colour.
};

using InspectorFlags = ImU64;
enum InspectorFlags_ {
    InspectorFlags_ShowWrap             = 1 << 0,  // Draw beyong the [0,1] uv range. What you see will depend on API
    InspectorFlags_NoForceFilterNearest = 1 << 1,  // Normally we force nearest neighbour sampling when zoomed in. Set to disable this.
    InspectorFlags_NoGrid               = 1 << 2,  // By default a grid is shown at high zoom levels
    InspectorFlags_NoTooltip            = 1 << 3,  // Disable tooltip on hover
    InspectorFlags_FillHorizontal       = 1 << 4,  // Scale to fill available space horizontally
    InspectorFlags_FillVertical         = 1 << 5,  // Scale to fill available space vertically
    InspectorFlags_FlipX                = 1 << 7,  // Horizontally flip the way the texture is displayed
    InspectorFlags_FlipY                = 1 << 8,  // Vertically flip the way the texture is displayed
};

void CurrentInspector_Recenter();
void CurrentInspector_Fit();
void CurrentInspector_Fill();

/* Use one of these Size structs if you want to specify an exact size for the inspector panel. 
 * E.g.
 * BeginInspectorPanel("MyPanel", texture_1K, ImVec2(1024,1024), 0, SizeExcludingBorder(ImVec2(1024,1024)));
 *
 * However, most of the time the default size will be fine. E.g.
 *
 * BeginInspectorPanel("MyPanel", texture_1K, ImVec2(1024,1024));
 */
struct SizeIncludingBorder {ImVec2 Size; SizeIncludingBorder(ImVec2 size):Size(size){}};
struct SizeExcludingBorder {ImVec2 size; SizeExcludingBorder(ImVec2 size):size(size){}};
/* BeginInspectorPanel
 * Returns true if panel is drawn.  Note that flags will only be considered on the first call */
bool BeginInspectorPanel(const char *name, const wt::gui::gl_image_t&,
                         InspectorFlags flags, SizeIncludingBorder size,
                         ImFont* tooltip_font);
bool BeginInspectorPanel(const char *name, const wt::gui::gl_image_t&,
                         InspectorFlags flags, SizeExcludingBorder size,
                         ImFont* tooltip_font);

bool BeginInspectorPanel(const char *name, const wt::gui::gl_images_t&,
                         InspectorFlags flags, SizeIncludingBorder size,
                         ImFont* tooltip_font);
bool BeginInspectorPanel(const char *name, const wt::gui::gl_images_t&,
                         InspectorFlags flags, SizeExcludingBorder size,
                         ImFont* tooltip_font);

/* EndInspectorPanel 
 * Always call after BeginInspectorPanel and after you have drawn any required annotations*/
void EndInspectorPanel();

/* ReleaseInspectorData
 */
void ReleaseInspectorData(ImGuiID id);

//-------------------------------------------------------------------------
// [SECTION] CURRENT INSPECTOR MANIPULATORS
//-------------------------------------------------------------------------
/* All the functions starting with CurrentInspector_ can be used after calling 
 * BeginInspector until the end of the frame.  It is not necessary to call them 
 * before the matching EndInspectorPanel
 */

static constexpr std::array<tinycolormap::ColormapType,6> colourmaps = {
    tinycolormap::ColormapType::Magma,
    tinycolormap::ColormapType::Turbo,
    tinycolormap::ColormapType::Inferno,
    tinycolormap::ColormapType::Plasma,
    tinycolormap::ColormapType::Viridis,
    tinycolormap::ColormapType::Cividis
};

void CurrentInspector_SetAlphaMode(InspectorAlphaMode);  
void CurrentInspector_SetFlags(InspectorFlags toSet, InspectorFlags toClear = 0);
inline void CurrentInspector_ClearFlags(InspectorFlags toClear) {CurrentInspector_SetFlags(0, toClear);}
void CurrentInspector_SetGridColor(ImU32 color);
void CurrentInspector_SetMaxAnnotations(int maxAnnotations);

void CurrentInspector_SetLinearMode(float exposure);
void CurrentInspector_SetGammaMode(bool sRGB, float gamma, float exposure);
void CurrentInspector_SetDBMode(float db_min, float db_max);
void CurrentInspector_SetFCMode(float min, float max, int channel);
void CurrentInspector_SetColourmap(int id);

void CurrentInspector_SetPolDOPMode();
void CurrentInspector_SetPolDOLPMode();
void CurrentInspector_SetPolDOCPMode();
void CurrentInspector_SetPolLPDirMode();
void CurrentInspector_SetPolErrorMode();
void CurrentInspector_SetPolFilterMode();

void CurrentInspector_SetRGBResponse(const wt::sensor::response::RGB_t*);

void CurrentInspector_SetStokesFilter(const ImVec4&);

/* CurrentInspector_SetCustomBackgroundColor
 * If using InspectorAlphaMode_CustomColor then this is the color that will be 
 * blended as the background where alpha is less than one.
 */
void CurrentInspector_SetCustomBackgroundColor(ImVec4 color);
void CurrentInspector_SetCustomBackgroundColor(ImU32 color);

/* CurrentInspector_GetID
 * Get the ID of the current inspector.  Currently only used for calling
 * ReleaseInspectorData. 
 */
ImGuiID CurrentInspector_GetID();


//-------------------------------------------------------------------------
// [SECTION] CONTEXT-WIDE SETTINGS
//-------------------------------------------------------------------------
/* SetZoomRate
 * factor should be greater than 1.  A value of 1.5 means one mouse wheel 
 * scroll will increase zoom level by 50%. The factor used for zooming out is 
 * 1/factor. */
void SetZoomRate(float factor); 
                                
//-------------------------------------------------------------------------
// [SECTION] Annotation Classes
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
// [SECTION] INTERNAL
//-------------------------------------------------------------------------

struct Transform2D {
    ImVec2 Scale;
    ImVec2 Translate;

    /* Transform a vector by this transform.  Scale is applied first */
    ImVec2 operator*(const ImVec2 &rhs) const {
        return ImVec2(Scale.x * rhs.x + Translate.x, Scale.y * rhs.y + Translate.y);
    }

    /* Return an inverse transform such that transform.Inverse() * transform * vector == vector*/
    [[nodiscard]] Transform2D Inverse() const {
        ImVec2 inverseScale(1 / Scale.x, 1 / Scale.y);
        return {inverseScale, ImVec2(-inverseScale.x * Translate.x, -inverseScale.y * Translate.y)};
    }
};

//-------------------------------------------------------------------------
// [SECTION] TEMPLATE IMPLEMENTATION
//-------------------------------------------------------------------------
void DrawAnnotations(ImU64 maxAnnotatedTexels = 0);

} // namespace ImGuiTexInspect
