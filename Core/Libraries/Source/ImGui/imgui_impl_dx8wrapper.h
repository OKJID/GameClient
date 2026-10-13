#pragma once

#include "imgui.h"

bool ImGui_ImplDX8Wrapper_Init();
void ImGui_ImplDX8Wrapper_Shutdown();
void ImGui_ImplDX8Wrapper_NewFrame();
void ImGui_ImplDX8Wrapper_RenderDrawData(ImDrawData* drawData);

ImTextureID ImGui_ImplDX8Wrapper_CreateTexture(int width, int height, const void* bgraPixels, int pitch);
void ImGui_ImplDX8Wrapper_DestroyTexture(ImTextureID textureId);
