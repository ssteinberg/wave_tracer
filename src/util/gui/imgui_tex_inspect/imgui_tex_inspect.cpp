// ImGuiTexInspect, a texture inspector widget for dear imgui
// adapted from https://github.com/andyborrell/imgui_tex_inspect/

//-------------------------------------------------------------------------
// [SECTION] INCLUDES
//-------------------------------------------------------------------------
#include <algorithm>
#include <numbers>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <wt/util/gui/img_inspect/imgui_tex_inspect.h>
#include <wt/util/gui/img_inspect/imgui_tex_inspect_internal.h>

#include <wt/util/gui/utils.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#if defined(_MSC_VER)
#pragma warning(disable : 4996) // 'sprintf' considered unsafe
#endif

namespace ImGuiTexInspect {

//-------------------------------------------------------------------------
// [SECTION] FORWARD DECLARATIONS
//-------------------------------------------------------------------------
void ColorTooltip(const char* text, ImFont* font, ImVec4 value, const Inspector *inspector);
void GetVisibleTexelRegion(Inspector *inspector, ImVec2 &texelTL, ImVec2 &texelBR);
void UpdateShaderOptions(Inspector *inspector);
void InspectorDrawCallback(const ImDrawList *parent_list, const ImDrawCmd *cmd);

//-------------------------------------------------------------------------
// [SECTION] GLOBAL STATE
//-------------------------------------------------------------------------

// Input mapping structure, default values listed in the comments.
struct InputMap {
    ImGuiMouseButton PanButton; // LMB      enables panning when held
    InputMap();
};

InputMap::InputMap() {
    PanButton = ImGuiMouseButton_Left;
}

// Settings configured via SetNextPanelOptions etc.
struct NextPanelSettings {
    InspectorFlags ToSet = 0;
    InspectorFlags ToClear = 0;
};

// Main context / configuration structure for imgui_tex_inspect
struct Context {
    InputMap                                    Input;                           // Input mapping config
    ImGuiStorage                                Inspectors;                      // All the inspectors we've seen
    Inspector *                                 CurrentInspector;                // Inspector currently being processed
    float                                       ZoomRate                 = 1.3f; // How fast mouse wheel affects zoom
    float                                       DefaultPanelHeight       = 600;  // Height of panel in pixels
    float                                       DefaultInitialPanelWidth = 600;  // Only applies when window first appears
    int                                         MaxAnnotations           = 1000; // Limit number of texel annotations for performance
};

static constexpr int borderWidth = 0;

Context *GContext = nullptr;

//-------------------------------------------------------------------------
// [SECTION] USER FUNCTIONS
//-------------------------------------------------------------------------

void Init() {
    // Nothing to do here.  But there might be in a later version. So client code should still call it!
}

void Shutdown() {
    // Nothing to do here.  But there might be in a later version. So client code should still call it!
}

Context *CreateContext() {
    GContext = IM_NEW(Context);
    SetCurrentContext(GContext);
    return GContext;
}

void DestroyContext(Context *ctx) {
    if (ctx == NULL) {
        ctx = GContext;
    }

    if (ctx == GContext) {
        GContext = NULL;
    }

    for (ImGuiStoragePair &pair : ctx->Inspectors.Data) {
        Inspector *inspector = (Inspector *)pair.val_p;
        if (inspector) {
            IM_DELETE(inspector);
        }
    }

    IM_DELETE(ctx);
}


void SetCurrentContext(Context *context) {
    ImGuiTexInspect::GContext = context;
}

void CurrentInspector_Recenter() {
    Inspector *inspector = GContext->CurrentInspector;

    const auto& panelSize = inspector->PanelSize;
    const auto& textureSize = inspector->texture_size();
    ImVec2 availablePanelSize = panelSize - ImVec2(borderWidth, borderWidth) * 2;

    auto scale = std::min<float>(1, 
        std::min<float>(availablePanelSize.x / textureSize.x, availablePanelSize.y / textureSize.y));
    if (textureSize.x==0 || textureSize.y==0)
        scale=1;

    inspector->Scale = ImVec2(scale, scale);
    SetPanPos(inspector, ImVec2(0.5f, 0.5f));
}

void CurrentInspector_Fit() {
    Inspector *inspector = GContext->CurrentInspector;

    const auto& panelSize = inspector->PanelSize;
    const auto& textureSize = inspector->texture_size();
    ImVec2 availablePanelSize = panelSize - ImVec2(borderWidth, borderWidth) * 2;

    auto scale = std::min<float>(availablePanelSize.x / textureSize.x, availablePanelSize.y / textureSize.y);
    if (textureSize.x==0 || textureSize.y==0)
        scale=1;

    inspector->Scale = ImVec2(scale, scale);
    SetPanPos(inspector, ImVec2(0.5f, 0.5f));
}

void CurrentInspector_Fill() {
    Inspector *inspector = GContext->CurrentInspector;

    const auto& panelSize = inspector->PanelSize;
    const auto& textureSize = inspector->texture_size();
    ImVec2 availablePanelSize = panelSize - ImVec2(borderWidth, borderWidth) * 2;

    auto scale = std::max<float>(availablePanelSize.x / textureSize.x, availablePanelSize.y / textureSize.y);
    if (textureSize.x==0 || textureSize.y==0)
        scale=1;

    inspector->Scale = ImVec2(scale, scale);
    SetPanPos(inspector, ImVec2(0.5f, 0.5f));
}

inline bool create_context(const char *title) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    const ImGuiID ID = window->GetID(title);

    Context *ctx = GContext;
    bool justCreated = GetByKey(ctx, ID) == NULL;
    ctx->CurrentInspector = GetOrAddByKey(ctx, ID);
    Inspector *inspector = ctx->CurrentInspector;
    justCreated |= !inspector->Initialized;

    inspector->ID = ID;

    return justCreated;
}

bool BeginInspectorPanel(const char *title,
                         InspectorFlags flags,
                         bool justCreated,
                         SizeIncludingBorder sizeIncludingBorder, ImFont* tooltip_font);

bool BeginInspectorPanel(const char *title,
                         InspectorFlags flags,
                         bool justCreated,
                         SizeExcludingBorder size, ImFont* tooltip_font) {
    // Correct the size to include the border, but preserve 0 which has a special meaning
    return BeginInspectorPanel(
            title, flags, justCreated,
            SizeIncludingBorder{ImVec2{size.size.x == 0 ? 0 : size.size.x + 2*borderWidth, 
                                          size.size.y == 0 ? 0 : size.size.y + 2*borderWidth}},
            tooltip_font);
}

bool BeginInspectorPanel(const char *name, const wt::gui::gl_image_t& glimage,
                         InspectorFlags flags, SizeIncludingBorder size, ImFont* tooltip_font) {
    const bool just_created = create_context(name);
    Inspector *inspector = GContext->CurrentInspector;
    
    inspector->image = glimage.image;
    inspector->Textures[0] = glimage ? (ImTextureID)glimage : 0;
    inspector->Stokes = false;
    inspector->ActiveShaderOptions.is_polarimetric = 0;

    return BeginInspectorPanel(name, flags, just_created, size, tooltip_font);
}
bool BeginInspectorPanel(const char *name, const wt::gui::gl_image_t& glimage,
                         InspectorFlags flags, SizeExcludingBorder size, ImFont* tooltip_font) {
    const bool just_created = create_context(name);
    Inspector *inspector = GContext->CurrentInspector;
    
    inspector->image = glimage.image;
    inspector->Textures[0] = glimage ? (ImTextureID)glimage : 0;
    inspector->Stokes = false;
    inspector->ActiveShaderOptions.is_polarimetric = 0;

    return BeginInspectorPanel(name, flags, just_created, size, tooltip_font);
}

bool BeginInspectorPanel(const char *name, const wt::gui::gl_images_t& glimages,
                         InspectorFlags flags, SizeIncludingBorder size, ImFont* tooltip_font) {
    const bool just_created = create_context(name);
    Inspector *inspector = GContext->CurrentInspector;
    
    inspector->images = glimages.images;
    if (glimages)
        inspector->Textures = { glimages[0],glimages[1],glimages[2],glimages[3] };
    else
        inspector->Textures = { 0,0,0,0 };
    inspector->Stokes = true;
    inspector->ActiveShaderOptions.is_polarimetric = 1;

    return BeginInspectorPanel(name, flags, just_created, size, tooltip_font);
}
bool BeginInspectorPanel(const char *name, const wt::gui::gl_images_t& glimages,
                         InspectorFlags flags, SizeExcludingBorder size, ImFont* tooltip_font) {
    const bool just_created = create_context(name);
    Inspector *inspector = GContext->CurrentInspector;
    
    inspector->images = glimages.images;
    if (glimages)
        inspector->Textures = { glimages[0],glimages[1],glimages[2],glimages[3] };
    else
        inspector->Textures = { 0,0,0,0 };
    inspector->Stokes = true;
    inspector->ActiveShaderOptions.is_polarimetric = 1;

    return BeginInspectorPanel(name, flags, just_created, size, tooltip_font);
}

bool BeginInspectorPanel(const char *title,
                         InspectorFlags flags,
                         bool justCreated,
                         SizeIncludingBorder sizeIncludingBorder,
                         ImFont* tooltip_font) {
    // Unpack size param.  It's in the SizeIncludingBorder structure just to make sure users know what they're requesting
    ImVec2 size = sizeIncludingBorder.Size;

    Context *ctx = GContext;
    Inspector *inspector = ctx->CurrentInspector;
    const ImGuiIO &IO = ImGui::GetIO();


    // Cache the basics
    const auto& textureSize = inspector->texture_size();
    inspector->Initialized = true;

    // Handle incoming flags. We keep special track of the 
    // newly set flags because somethings only take effect
    // the first time the flag is set.
    InspectorFlags newlySetFlags = {};
    if (justCreated) {
        SetFlag(newlySetFlags, flags);
        inspector->MaxAnnotatedTexels = ctx->MaxAnnotations;
    }
    inspector->Flags = flags;

    // Calculate panel size
    ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();

    ImVec2 panelSize;
    // A size value of zero indicates we should use defaults
    if (justCreated) {
        panelSize = {size.x == 0 ? ImMax(ctx->DefaultInitialPanelWidth, contentRegionAvail.x) : size.x,
                     size.y == 0 ? ctx->DefaultPanelHeight : size.y};
    }
    else {
        panelSize = {size.x == 0 ? contentRegionAvail.x : size.x, size.y == 0 ? ctx->DefaultPanelHeight : size.y};
    }

    inspector->PanelSize = panelSize;
    ImVec2 availablePanelSize = panelSize - ImVec2(borderWidth, borderWidth) * 2;
 {
        // Possibly update scale
        float newScale = -1;

        if (HasFlag(newlySetFlags, InspectorFlags_FillVertical)) {
            newScale = availablePanelSize.y / textureSize.y;
        }
        else if (HasFlag(newlySetFlags, InspectorFlags_FillHorizontal)) {
            newScale = availablePanelSize.x / textureSize.x;
        }
        else if (justCreated) {
            newScale = 1;
        }

        if (newScale != -1) {
            inspector->Scale = ImVec2(newScale, newScale);
            SetPanPos(inspector, ImVec2(0.5f, 0.5f));
        }
    }

    RoundPanPos(inspector);

    ImVec2 textureSizePixels = inspector->Scale * textureSize;  // Size whole texture would appear on screen
    ImVec2 viewSizeUV = availablePanelSize / textureSizePixels; // Cropped size in terms of UV
    ImVec2 uv0 = inspector->PanPos - viewSizeUV * 0.5;
    ImVec2 uv1 = inspector->PanPos + viewSizeUV * 0.5;

    ImVec2 drawImageOffset{borderWidth, borderWidth};
    ImVec2 viewSize = availablePanelSize;

    if ((inspector->Flags & InspectorFlags_ShowWrap) == 0) {
        /* Don't crop the texture to UV [0,1] range.  What you see outside this 
         * range will depend on API and texture properties */
        if (textureSizePixels.x < availablePanelSize.x) {
            // Not big enough to horizontally fill view
            viewSize.x = ImFloor(textureSizePixels.x);
            drawImageOffset.x += ImFloor((availablePanelSize.x - textureSizePixels.x) / 2);
            uv0.x = 0;
            uv1.x = 1;
            viewSizeUV.x = 1;
            inspector->PanPos.x = 0.5f;
        }
        if (textureSizePixels.y < availablePanelSize.y) {
            // Not big enough to vertically fill view
            viewSize.y = ImFloor(textureSizePixels.y);
            drawImageOffset.y += ImFloor((availablePanelSize.y - textureSizePixels.y) / 2);
            uv0.y = 0;
            uv1.y = 1;
            viewSizeUV.y = 1;
            inspector->PanPos.y = 0.5;
        }
    }

    if (HasFlag(flags,InspectorFlags_FlipX)) {
        ImSwap(uv0.x, uv1.x);
        viewSizeUV.x *= -1;
    }

    if (HasFlag(flags,InspectorFlags_FlipY)) {
        ImSwap(uv0.y, uv1.y);
        viewSizeUV.y *= -1;
    }

    inspector->ViewSize = viewSize;
    inspector->ViewSizeUV = viewSizeUV;

    /* We use mouse scroll to zoom so we don't want scroll to propagate to 
     * parent window. For this to happen we must NOT set 
     * ImGuiWindowFlags_NoScrollWithMouse.  This seems strange but it's the way 
     * ImGui works.  Also we must ensure the ScrollMax.y is not zero for the 
     * child window. */
    if (ImGui::BeginChild(title, panelSize, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove)) {
        // See comment above
        ImGui::GetCurrentWindow()->ScrollMax.y = 1.0f;

        // Callback for using our own image shader 
        ImGui::GetWindowDrawList()->AddCallback(InspectorDrawCallback, inspector);

        // Keep track of size of area that we draw for borders later
        inspector->PanelTopLeftPixel = ImGui::GetCursorScreenPos();
        ImGui::SetCursorPos(ImGui::GetCursorPos() + drawImageOffset);
        inspector->ViewTopLeftPixel = ImGui::GetCursorScreenPos();

        UpdateShaderOptions(inspector);
        inspector->CachedShaderOptions = inspector->ActiveShaderOptions;
        ImGui::Image(inspector->Textures[0], viewSize, uv0, uv1);
        ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

        /* Matrices for going back and forth between texel coordinates in the 
         * texture and screen coordinates based on where texture is drawn. 
         * Useful for annotations and mouse hover etc. */
        inspector->TexelsToPixels = GetTexelsToPixels(inspector->ViewTopLeftPixel, viewSize, uv0, viewSizeUV, textureSize);
        inspector->PixelsToTexels = inspector->TexelsToPixels.Inverse();

        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 mousePosTexel = inspector->PixelsToTexels * mousePos;
        ImVec2 mouseUV = mousePosTexel / textureSize;
        const bool mouse_outofbounds =
            mousePosTexel.x>textureSize.x || mousePosTexel.x<0 ||
            mousePosTexel.y>textureSize.y || mousePosTexel.y<0;
        mousePosTexel.x = Modulus(mousePosTexel.x, textureSize.x);
        mousePosTexel.y = Modulus(mousePosTexel.y, textureSize.y);

        if (ImGui::IsItemHovered() && (inspector->Flags & ImGuiTexInspect::InspectorFlags_NoTooltip) == 0) {
            // Show a tooltip for currently hovered texel
            if ((inspector->image || inspector->images) && !mouse_outofbounds) {
                ImVec4 color = inspector->get_texel((int)mousePosTexel.x, (int)mousePosTexel.y);

                char buffer[128];
                sprintf(buffer, "Texel: (%d, %d)", (int)mousePosTexel.x, (int)mousePosTexel.y);

                ColorTooltip(buffer, tooltip_font, color, inspector);
            }
        }

        bool hovered = ImGui::IsWindowHovered();
        {  //DRAGGING
            
            // start drag
            if (!inspector->IsDragging && hovered && IO.MouseClicked[ctx->Input.PanButton]) {
                inspector->IsDragging = true;
            }
            // carry on dragging
            else if (inspector->IsDragging) {
                ImVec2 uvDelta = IO.MouseDelta * viewSizeUV / viewSize;
                inspector->PanPos -= uvDelta;
                RoundPanPos(inspector);
            }

            // end drag
            if (inspector->IsDragging && (IO.MouseReleased[ctx->Input.PanButton] || !IO.MouseDown[ctx->Input.PanButton])) {
                inspector->IsDragging = false;
            }
        }

        // ZOOM
        if (hovered && IO.MouseWheel != 0) {
            float zoomRate  = ctx->ZoomRate;
            float scale     = inspector->Scale.y;
            float prevScale = scale;

            bool keepTexelSizeRegular = scale > inspector->MinimumGridSize && !HasFlag(inspector->Flags, InspectorFlags_NoGrid);
            if (IO.MouseWheel > 0) {
                scale *= zoomRate;
                if (keepTexelSizeRegular) {
                    // It looks nicer when all the grid cells are the same size
                    // so keep scale integer when zoomed in
                    scale = ImCeil(scale);
                }
            }
            else {
                scale /= zoomRate;
                if (keepTexelSizeRegular) {
                    // See comment above. We're doing a floor this time to make
                    // sure the scale always changes when scrolling
                    scale = ImFloor(scale);
                }
            }
            /* To make it easy to get back to 1:1 size we ensure that we stop 
             * here without going straight past it*/
            if ((prevScale < 1 && scale > 1) || (prevScale > 1 && scale < 1)) {
                scale = 1;
            }
            SetScale(inspector, ImVec2(scale, scale));
            SetPanPos(inspector, inspector->PanPos + (mouseUV - inspector->PanPos) * (1 - prevScale / scale));
        }

        return true;
    }
    else {
        return false;
    }
}

void EndInspectorPanel() {
    const ImU32 innerBorderColour = 0xFFFFFFFF;
    const ImU32 outerBorderColour = 0xFF888888;
    Inspector *inspector = GContext->CurrentInspector;

    if (borderWidth>0) {
        // Draw out border around whole inspector panel
        ImGui::GetWindowDrawList()->AddRect(inspector->PanelTopLeftPixel, inspector->PanelTopLeftPixel + inspector->PanelSize,
                                            outerBorderColour);

        // Draw innder border around texture.  If zoomed in this will completely cover the outer border
        ImGui::GetWindowDrawList()->AddRect(inspector->ViewTopLeftPixel - ImVec2(borderWidth,borderWidth),
                                            inspector->ViewTopLeftPixel + inspector->ViewSize + ImVec2(borderWidth,borderWidth), innerBorderColour);
    }

    ImGui::EndChild();
}

void ReleaseInspectorData(ImGuiID ID) {
    Inspector *inspector = GetByKey(GContext, ID);

    if (inspector == NULL)
        return;

    /* In a later version we will remove inspector from the inspector table 
     * altogether. For now we reset the whole inspector structure to prevent 
     * clients relying on persisted data. 
     */
    *inspector = Inspector();
}


ImGuiID CurrentInspector_GetID() {
    return GContext->CurrentInspector->ID;
}

void CurrentInspector_SetAlphaMode(InspectorAlphaMode mode) {
    Inspector *inspector = GContext->CurrentInspector;
    ShaderOptions *shaderOptions = &inspector->ActiveShaderOptions;

    inspector->AlphaMode = mode;

    switch (mode) {
    case InspectorAlphaMode_Black:
        shaderOptions->BackgroundColor = ImVec4(0, 0, 0, 1);
        shaderOptions->DisableFinalAlpha = 1;
        shaderOptions->PremultiplyAlpha = 1;
        break;
    case InspectorAlphaMode_White:
        shaderOptions->BackgroundColor = ImVec4(1, 1, 1, 1);
        shaderOptions->DisableFinalAlpha = 1;
        shaderOptions->PremultiplyAlpha = 1;
        break;
    case InspectorAlphaMode_ImGui:
        shaderOptions->BackgroundColor = ImVec4(0, 0, 0, 0);
        shaderOptions->DisableFinalAlpha = 0;
        shaderOptions->PremultiplyAlpha = 0;
        break;
    case InspectorAlphaMode_CustomColor:
        shaderOptions->BackgroundColor = inspector->CustomBackgroundColor;
        shaderOptions->DisableFinalAlpha = 1;
        shaderOptions->PremultiplyAlpha = 1;
        break;
    }
}

void CurrentInspector_SetFlags(InspectorFlags toSet, InspectorFlags toClear) {
    Inspector *inspector = GContext->CurrentInspector;
    SetFlag(inspector->Flags, toSet);
    ClearFlag(inspector->Flags, toClear);
}

void CurrentInspector_SetGridColor(ImU32 color) {
    Inspector *inspector = GContext->CurrentInspector;
    float alpha = inspector->ActiveShaderOptions.GridColor.w;
    inspector->ActiveShaderOptions.GridColor = ImColor(color);
    inspector->ActiveShaderOptions.GridColor.w = alpha;
}

void CurrentInspector_SetLinearMode(float exposure) {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->ActiveShaderOptions.Mode = Mode_e::linear;
    inspector->ActiveShaderOptions.ModeData = ImVec4( exposure,0,0, inspector->is_rgb_image()?0:1);
}
void CurrentInspector_SetGammaMode(bool sRGB, float gamma, float exposure) {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->ActiveShaderOptions.Mode = Mode_e::gamma;
    inspector->ActiveShaderOptions.ModeData = ImVec4( exposure, sRGB ? -1 : gamma,0, inspector->is_rgb_image()?0:1);
}
void CurrentInspector_SetDBMode(float db_min, float db_max) {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->ActiveShaderOptions.Mode = Mode_e::db;
    inspector->ActiveShaderOptions.ModeData = ImVec4( db_min,db_max,0, inspector->is_rgb_image()?0:1);
}
void CurrentInspector_SetFCMode(float min, float max, int channel) {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->ActiveShaderOptions.Mode = Mode_e::fc;
    inspector->ActiveShaderOptions.ModeData = ImVec4(glm::intBitsToFloat(channel), min,max, inspector->is_rgb_image()?0:1);
}

void CurrentInspector_SetPolDOPMode() {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->ActiveShaderOptions.PolMode = PolMode_e::DOP;
}
void CurrentInspector_SetPolDOLPMode() {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->ActiveShaderOptions.PolMode = PolMode_e::DOLP;
}
void CurrentInspector_SetPolDOCPMode() {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->ActiveShaderOptions.PolMode = PolMode_e::DOCP;
}
void CurrentInspector_SetPolFilterMode() {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->ActiveShaderOptions.PolMode = PolMode_e::pol_filter;
}
void CurrentInspector_SetPolLPDirMode() {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->ActiveShaderOptions.PolMode = PolMode_e::LPDir;
}
void CurrentInspector_SetPolErrorMode() {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->ActiveShaderOptions.PolMode = PolMode_e::Error;
}

void CurrentInspector_SetStokesFilter(const ImVec4& s) {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->ActiveShaderOptions.stokes_filter = s;
}

void CurrentInspector_SetColourmap(int id) {
    assert(id>=0 && id<6);
    Inspector *inspector = GContext->CurrentInspector;
    if (id>=0 && id<6)
        inspector->ActiveShaderOptions.colourmap = id;
}

void CurrentInspector_SetRGBResponse(const wt::sensor::response::RGB_t* rgb) {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->rgb_response_function = rgb;
    inspector->XYZ_to_RGB = {};
    inspector->RGB_to_XYZ = {};
    if (rgb) {
        inspector->XYZ_to_RGB = rgb->get_XYZ_to_RGB_matrix();
        inspector->RGB_to_XYZ = wt::m::inverse(inspector->XYZ_to_RGB);
    }
}

void CurrentInspector_SetMaxAnnotations(int maxAnnotations) {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->MaxAnnotatedTexels = maxAnnotations;
}

void CurrentInspector_SetCustomBackgroundColor(ImVec4 color) {
    Inspector *inspector = GContext->CurrentInspector;
    inspector->CustomBackgroundColor = color;
    if (inspector->AlphaMode == InspectorAlphaMode_CustomColor) {
        inspector->ActiveShaderOptions.BackgroundColor = color;
    }
}

void CurrentInspector_SetCustomBackgroundColor(ImU32 color) {
    CurrentInspector_SetCustomBackgroundColor(ImGui::ColorConvertU32ToFloat4(color));
}

void SetZoomRate(float rate) {
    GContext->ZoomRate = rate;
}

//-------------------------------------------------------------------------
// [SECTION] Scaling and Panning
//-------------------------------------------------------------------------
void RoundPanPos(Inspector *inspector) {
    if ((inspector->Flags & InspectorFlags_ShowWrap) > 0) {
        /* PanPos is the point in the center of the current view. Allow the 
         * user to pan anywhere as long as the view center is inside the 
         * texture.*/
        inspector->PanPos = ImClamp(inspector->PanPos, ImVec2(0, 0), ImVec2(1, 1));
    }
    else {
        /* When ShowWrap mode is disabled the limits are a bit more strict. We 
         * try to keep it so that the user cannot pan past the edge of the 
         * texture at all.*/
        ImVec2 absViewSizeUV = Abs(inspector->ViewSizeUV);
        inspector->PanPos = ImMax(inspector->PanPos - absViewSizeUV / 2, ImVec2(0, 0)) + absViewSizeUV / 2;
        inspector->PanPos = ImMin(inspector->PanPos + absViewSizeUV / 2, ImVec2(1, 1)) - absViewSizeUV / 2;
    }

    const auto& texture_size = inspector->texture_size();

    /* If inspector->scale is 1 then we should ensure that pixels are aligned 
     * with texel centers to get pixel-perfect texture rendering*/
    ImVec2 topLeftSubTexel = inspector->PanPos * inspector->Scale * texture_size - inspector->ViewSize * 0.5f;

    if (inspector->Scale.x >= 1) {
        topLeftSubTexel.x = Round(topLeftSubTexel.x);
    }
    if (inspector->Scale.y >= 1) {
        topLeftSubTexel.y = Round(topLeftSubTexel.y);
    }
    inspector->PanPos = (topLeftSubTexel + inspector->ViewSize * 0.5f) / (inspector->Scale * texture_size);
}

void SetPanPos(Inspector *inspector, ImVec2 pos) {
    inspector->PanPos = pos;
    RoundPanPos(inspector);
}

void SetScale(Inspector *inspector, ImVec2 scale) {
    scale = ImClamp(scale, inspector->ScaleMin, inspector->ScaleMax);

    inspector->ViewSizeUV *= inspector->Scale / scale;

    inspector->Scale = scale;
    
    // Only force nearest sampling if zoomed in
    inspector->ActiveShaderOptions.ForceNearestSampling =
        (inspector->Scale.x > 1.0f || inspector->Scale.y > 1.0f) && !HasFlag(inspector->Flags, InspectorFlags_NoForceFilterNearest);
    inspector->ActiveShaderOptions.GridWidth = ImVec2(1.0f / inspector->Scale.x, 1.0f / inspector->Scale.y);
}

void SetScale(Inspector *inspector, float scaleY) {
    SetScale(inspector, ImVec2(scaleY, scaleY));
}
//-------------------------------------------------------------------------
// [SECTION] INSPECTOR MAP
//-------------------------------------------------------------------------

Inspector *GetByKey(const Context *ctx, ImGuiID key) {
    return (Inspector *)ctx->Inspectors.GetVoidPtr(key);
}

Inspector *GetOrAddByKey(Context *ctx, ImGuiID key) {
    Inspector *inspector = GetByKey(ctx, key);
    if (inspector) {
        return inspector;
    }
    else {
        inspector = IM_NEW(Inspector);
        ctx->Inspectors.SetVoidPtr(key, inspector);
        return inspector;
    }
}

//-------------------------------------------------------------------------
// [SECTION] TextureConversion class
//-------------------------------------------------------------------------

ShaderOptions::ShaderOptions() = default;

//-------------------------------------------------------------------------
// [SECTION] UI and CONFIG
//-------------------------------------------------------------------------

void UpdateShaderOptions(Inspector *inspector) {
    if (HasFlag(inspector->Flags, InspectorFlags_NoGrid) == false && inspector->Scale.y > inspector->MinimumGridSize) {
        // Enable grid in shader
        inspector->ActiveShaderOptions.GridColor.w = 1;
        SetScale(inspector, Round(inspector->Scale.y));
    }
    else {
        // Disable grid in shader
        inspector->ActiveShaderOptions.GridColor.w = 0;
    }

    inspector->ActiveShaderOptions.ForceNearestSampling =
        (inspector->Scale.x > 1.0f || inspector->Scale.y > 1.0f) && !HasFlag(inspector->Flags, InspectorFlags_NoForceFilterNearest);
}

const ImGuiCol disabledUIColorIds[] = {ImGuiCol_FrameBg, 
                                       ImGuiCol_FrameBgActive, 
                                       ImGuiCol_FrameBgHovered, 
                                       ImGuiCol_Text,
                                       ImGuiCol_CheckMark};

// Push disabled style for ImGui elements
void PushDisabled() {
    for (ImGuiCol colorId : disabledUIColorIds) {
        ImVec4 color = ImGui::GetStyleColorVec4(colorId);
        color = color * ImVec4(0.5f, 0.5f, 0.5f, 0.5f);
        ImGui::PushStyleColor(colorId, color);
    }
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
}

// Pop disabled style for ImGui elements
void PopDisabled() {
    for (ImGuiCol colorId : disabledUIColorIds) {
        (void)colorId;
        ImGui::PopStyleColor();
    }
    ImGui::PopItemFlag();
}

//-------------------------------------------------------------------------
// [SECTION] Rendering & Buffer Management
//-------------------------------------------------------------------------

void InspectorDrawCallback(const ImDrawList *parent_list, const ImDrawCmd *cmd) {
    // Forward call to API-specific backend
    Inspector *inspector = (Inspector *)cmd->UserCallbackData;
    BackEnd_SetShader(parent_list, cmd, inspector);
}

// Calculate a transform to convert from texel coordinates to screen pixel coordinates
Transform2D GetTexelsToPixels(ImVec2 screenTopLeft, ImVec2 screenViewSize, ImVec2 uvTopLeft, ImVec2 uvViewSize, ImVec2 textureSize) {
    ImVec2 uvToPixel = screenViewSize / uvViewSize;

    Transform2D transform;
    transform.Scale = uvToPixel / textureSize;
    transform.Translate.x = screenTopLeft.x - uvTopLeft.x * uvToPixel.x;
    transform.Translate.y = screenTopLeft.y - uvTopLeft.y * uvToPixel.y;
    return transform;
}

struct AnnotationsDesc {
    ImDrawList  *DrawList;
    ImVec2       TexelViewSize;  // How many texels are visible for annotating
    ImVec2       TexelTopLeft;   // Coordinated in texture space of top left visible texel
    Transform2D  TexelsToPixels; // Transform to go from texel space to screen pixel space
};

/* Fills in the AnnotationsDesc structure which provides all necessary 
 * information for code which draw annoations.  Returns false if no annoations 
 * should be drawn.  The maxAnnotatedTexels argument provides a way to override
 * the default maxAnnotatedTexels.
 */

//-------------------------------------------------------------------------
// [SECTION] FORWARD DECLARATIONS FOR TEMPLATE IMPLEMENTATION - Do not call directly
//-------------------------------------------------------------------------

bool GetAnnotationDesc(AnnotationsDesc *ad, ImU64 maxAnnotatedTexels) {
    Inspector *inspector = GContext->CurrentInspector;

    if (maxAnnotatedTexels == 0) {
        maxAnnotatedTexels = inspector->MaxAnnotatedTexels;
    }
    if (maxAnnotatedTexels != 0) {
        /* Check if we would draw too many annotations.  This is to avoid poor 
         * frame rate when too zoomed out.  Increase MaxAnnotatedTexels if you 
         * want to draw more annotations.  Note that we don't use texelTL & 
         * texelBR to get total visible texels as this would cause flickering 
         * while panning as the exact number of visible texels changes.
        */

        ImVec2 screenViewSizeTexels = Abs(inspector->PixelsToTexels.Scale) * inspector->ViewSize;
        ImU64 approxVisibleTexelCount = (ImU64)screenViewSizeTexels.x * (ImU64)screenViewSizeTexels.y;
        if (approxVisibleTexelCount > maxAnnotatedTexels) {
            return false;
        }
    }

    // texelTL & texelBL will describe the currently visible texel region
    ImVec2 texelTL;
    ImVec2 texelBR;

    GetVisibleTexelRegion(inspector, texelTL, texelBR);
 {
        ad->DrawList = ImGui::GetWindowDrawList();
        ad->TexelsToPixels = inspector->TexelsToPixels;
        ad->TexelTopLeft = texelTL;
        ad->TexelViewSize = texelBR - texelTL;
        return true;
    }

    return true;
}

/* Calculates currently visible region of texture (which is returned in texelTL 
 * and texelBR).
 */
void GetVisibleTexelRegion(Inspector *inspector, ImVec2 &texelTL, ImVec2 &texelBR) {
    /* Figure out which texels correspond to the top left and bottom right
     * corners of the texture view.  The plus + ImVec2(1,1) is because we
     * want to draw partially visible texels on the bottom and right edges.
     */
    texelTL = ImFloor(inspector->PixelsToTexels * inspector->ViewTopLeftPixel);
    texelBR = ImFloor(inspector->PixelsToTexels * (inspector->ViewTopLeftPixel + inspector->ViewSize));

    if (texelTL.x > texelBR.x) {
        ImSwap(texelTL.x, texelBR.x);
    }
    if (texelTL.y > texelBR.y) {
        ImSwap(texelTL.y, texelBR.y);
    }

    /* Add ImVec2(1,1) because we want to draw partially visible texels on the 
     * bottom and right edges.*/
    texelBR += ImVec2(1,1);

    const auto& texture_size = inspector->texture_size();

    texelTL = ImClamp(texelTL, ImVec2(0, 0), texture_size);
    texelBR = ImClamp(texelBR, ImVec2(0, 0), texture_size);
}


//-------------------------------------------------------------------------
// [SECTION] Annotations
//-------------------------------------------------------------------------

void ColorTooltip(const char* text, ImFont* font, ImVec4 value, const Inspector *inspector) {
    ImGuiContext& g = *GImGui;

    int flags = 0;

    if (!ImGui::BeginTooltipEx(ImGuiTooltipFlags_OverridePrevious, ImGuiWindowFlags_None))
        return;

    ImGui::PushFont(font, ImGui::GetFontSize());

    const char* text_end = text ? ImGui::FindRenderedTextEnd(text, NULL) : text;
    if (text_end > text) {
        ImGui::TextEx(text, text_end);
        ImGui::Separator();
    }

    const auto dbmode = inspector->ActiveShaderOptions.Mode == Mode_e::db;
    auto RGB = inspector->is_rgb_image();
    auto RGBA = inspector->is_rgba_image();

    auto orig_value = value;
    if (dbmode) {
        auto x = value.x;
        if (RGB) {
            orig_value.x = (orig_value.x+orig_value.y+orig_value.z) / 3.f;
            x = (value.x+value.y+value.z) / 3.f;
        }
        value.x = 10.f * std::log(x) / std::numbers::ln10_v<float>;

        RGB = false;
    }
    if (!RGB) {
        value.y = value.z = value.x;
    }

    ImVec4 cf(value.x, value.y, value.z, !inspector->is_rgba_image() ? 1.0f : value.w);

    if (dbmode) {
        const auto db_min = inspector->ActiveShaderOptions.ModeData.x;
        const auto db_max = inspector->ActiveShaderOptions.ModeData.y;
        const auto x = (value.x - db_min) / (db_max - db_min);

        const auto mapped = tinycolormap::GetColor(x, colourmaps[inspector->ActiveShaderOptions.colourmap]);
        cf.x = mapped[0];
        cf.y = mapped[1];
        cf.z = mapped[2];
        cf.w = 1;
    }

    ImVec2 sz(g.FontSize * 3 + g.Style.FramePadding.y * 2, g.FontSize * 3 + g.Style.FramePadding.y * 2);
    ImGuiColorEditFlags flags_to_forward = ImGuiColorEditFlags_InputMask_ | ImGuiColorEditFlags_AlphaMask_;

    ImGui::ColorButton("##inspect_tooltip_col_preview", cf,
                        (flags & flags_to_forward) | ImGuiColorEditFlags_NoTooltip, sz);
    ImGui::SameLine();

    char buffer[256];
    if (!dbmode) {
        if (inspector->rgb_response_function && RGB) {
            const auto XYZ = inspector->RGB_to_XYZ * wt::vec3_t{ value.x,value.y,value.z };
            const auto lab = wt::colourspace::XYZ_to_LAB(XYZ, inspector->rgb_response_function->get_whitepoint());
            if (RGBA)
                sprintf(buffer, 
                    "RGB: %5.3f, %5.3f, %5.3f\nLAB: %5.3f, %5.3f, %5.3f\nA: %5.3f",
                    value.x,value.y,value.z, lab.x,lab.y,lab.z, value.w);
            else
                sprintf(buffer, 
                    "RGB: %5.3f, %5.3f, %5.3f\nLAB: %5.3f, %5.3f, %5.3f",
                    value.x,value.y,value.z, lab.x,lab.y,lab.z);
        }
        else {
            sprintf(buffer, 
                RGB ? (RGBA ? "RGB: %5.3f, %5.3f, %5.3f\nA: %5.3f" : "RGB: %5.3f, %5.3f, %5.3f") :
                "%5.3f",
                value.x, value.y, value.z, value.w);
        }
    }
    else
        sprintf(buffer, "%5.3f dB\n%.4e", value.x, orig_value.x);
    float xoffset = 0;
    for (int i=0;buffer[i]!=0;++i) {
        if (buffer[i]=='i' && buffer[i+1]=='n' && buffer[i+2]=='f') {
            static const char* inf = "∞";
            buffer[i+0]=inf[0];
            buffer[i+1]=inf[1];
            buffer[i+2]=inf[2];
            i+=2;
            xoffset = 1.5f;
        }
    }
    if (xoffset>0)
        ImGui::SameLine(0,xoffset);
    ImGui::Text("%s",buffer);

    ImGui::PopFont();
    
    ImGui::EndTooltip();
}


void DrawAnnotation(ImDrawList *drawList, ImVec2 texel, Transform2D texelsToPixels, ImVec4 value) {
    Inspector *inspector = GContext->CurrentInspector;

    const auto dbmode = inspector->ActiveShaderOptions.Mode == Mode_e::db;
    const auto RGB = inspector->is_rgb_image();
    const auto RGBA = inspector->is_rgba_image();

    const auto TextColumnCount = dbmode ? 7 : 4;
    const auto TextRowCount = RGB ? 4 : 1;

    const char* TextFormatString;
    if (dbmode)
        TextFormatString = "\n%4.2f dB";
    else
        TextFormatString = RGB ? (RGBA ? "%5.3f\n%5.3f\n%5.3f\n%5.3f" : "%5.3f\n%5.3f\n%5.3f") : "\n%5.3f";

    char buffer[64];

    float fontHeight = ImGui::GetFontSize();
    float fontWidth = fontHeight / 1.4f; /* WARNING this is a hack that gets a constant
    * character width from half the height.  This work for the default font but
    * won't work on other fonts which may even not be monospace.*/

    // Calculate size of text and check if it fits
    ImVec2 textSize = ImVec2((float)TextColumnCount * fontWidth, (float)TextRowCount * fontHeight);

    if (textSize.x > ImAbs(texelsToPixels.Scale.x) || textSize.y > ImAbs(texelsToPixels.Scale.y)) {
        // Not enough room in texel to fit the text.  Don't draw it.
        return;
    }

    /* Choose black or white text based on how bright the texel.  I.e. don't 
     * draw black text on a dark background or vice versa. */
    ImU32 lineColor;
    if (dbmode) {
        auto x = value.x;
        if (RGB)
            x = (value.x+value.y+value.z) / 3.f;

        value.x = 10.f * std::log(x) / std::numbers::ln10_v<float>;
        value.y = value.z = value.x;

        const auto db_min = inspector->ActiveShaderOptions.ModeData.x;
        const auto db_max = inspector->ActiveShaderOptions.ModeData.y;
        const auto mapped = (value.x - db_min) / (db_max - db_min);

        lineColor = mapped>=.5 ? 0xFF000000 : 0xFFFFFFFF;
    } else {
        float brightness = RGB ? (value.x + value.y + value.z) * value.w / 3 : value.x*value.w;
        lineColor = brightness > 0.5 ? 0xFF000000 : 0xFFFFFFFF;
    }

    sprintf(buffer, TextFormatString, value.x, value.y, value.z, value.w);
    float xoffset = 0;
    for (int i=0;buffer[i]!=0;++i) {
        if (buffer[i]=='i' && buffer[i+1]=='n' && buffer[i+2]=='f') {
            static const char* inf = "∞";
            buffer[i+0]=inf[0];
            buffer[i+1]=inf[1];
            buffer[i+2]=inf[2];
            i+=2;
            xoffset = 1.5f;
        }
    }

    // Add text to drawlist!
    ImVec2 pixelCenter = texelsToPixels * texel;
    drawList->AddText(pixelCenter - textSize * 0.5f + ImVec2{ fontWidth * xoffset,0 }, lineColor, buffer);
}

void DrawAnnotations(ImU64 maxAnnotatedTexels) {
    Inspector *inspector = GContext->CurrentInspector;
    if (!inspector->image && !inspector->images)
        return;

    AnnotationsDesc ad;
    if (GetAnnotationDesc(&ad, maxAnnotatedTexels)) {
        ImVec2 texelBottomRight = ImVec2(ad.TexelTopLeft.x + ad.TexelViewSize.x, ad.TexelTopLeft.y + ad.TexelViewSize.y);
        for (int ty = (int)ad.TexelTopLeft.y; ty < (int)texelBottomRight.y; ++ty) {
            for (int tx = (int)ad.TexelTopLeft.x; tx < (int)texelBottomRight.x; ++tx) {
                ImVec4 color = inspector->get_texel(tx, ty);
                ImVec2 center = {(float)tx + 0.5f, (float)ty + 0.5f};
                DrawAnnotation(ad.DrawList, center, ad.TexelsToPixels, color);
            }
        }
    }
}

} // namespace ImGuiTexInspect
