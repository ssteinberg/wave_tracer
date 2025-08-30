// ImGuiTexInspect, a texture inspector widget for dear imgui
#pragma once

#include <cstddef>
namespace ImGuiTexInspect
{
bool ImplOpenGL3_Init(const char *glsl_version = NULL);
void ImplOpenGl3_Shutdown();
} // namespace ImGuiTexInspect
