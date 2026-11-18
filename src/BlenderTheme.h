#pragma once
#include "imgui.h"

namespace BlenderTheme {

inline void applyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Authentic Blender 4.2 LTS Dark Theme Palette
    colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f); // #E6E6E6
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.55f, 0.55f, 0.56f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.114f, 0.114f, 0.114f, 1.00f); // #1D1D1D (Workspace Gap / Base)
    colors[ImGuiCol_ChildBg]               = ImVec4(0.188f, 0.188f, 0.188f, 1.00f); // #303030 (Editor Panel Bg)
    colors[ImGuiCol_PopupBg]               = ImVec4(0.145f, 0.145f, 0.150f, 0.98f); // #252526 (Menu Popup)
    colors[ImGuiCol_Border]                = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Blender Widget Colors (#545454 idle, #656565 hover, #4772B3 active)
    colors[ImGuiCol_FrameBg]               = ImVec4(0.329f, 0.329f, 0.329f, 1.00f); // #545454
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.396f, 0.396f, 0.396f, 1.00f); // #656565
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.200f, 0.200f, 0.200f, 1.00f);

    colors[ImGuiCol_TitleBg]               = ImVec4(0.114f, 0.114f, 0.114f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.150f, 0.150f, 0.150f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.100f, 0.100f, 0.100f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.114f, 0.114f, 0.114f, 1.00f);

    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.14f, 0.14f, 0.14f, 0.45f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.32f, 0.32f, 0.32f, 0.85f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    // Blender Selection Blue (#4772B3) & Orange Accent (#E87D0D)
    colors[ImGuiCol_CheckMark]             = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.278f, 0.447f, 0.702f, 1.00f); // #4772B3
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.340f, 0.520f, 0.800f, 1.00f);

    colors[ImGuiCol_Button]                = ImVec4(0.329f, 0.329f, 0.329f, 1.00f); // #545454
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.400f, 0.400f, 0.400f, 1.00f); // #666666
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.278f, 0.447f, 0.702f, 1.00f); // #4772B3

    // Collapsing Header (#3D3D3D)
    colors[ImGuiCol_Header]                = ImVec4(0.239f, 0.239f, 0.239f, 1.00f); // #3D3D3D
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.290f, 0.290f, 0.290f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.278f, 0.447f, 0.702f, 1.00f); // #4772B3

    colors[ImGuiCol_Separator]             = ImVec4(0.114f, 0.114f, 0.114f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.278f, 0.447f, 0.702f, 0.85f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.278f, 0.447f, 0.702f, 1.00f);

    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.18f, 0.18f, 0.18f, 0.30f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.278f, 0.447f, 0.702f, 0.70f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.278f, 0.447f, 0.702f, 1.00f);

    colors[ImGuiCol_Tab]                   = ImVec4(0.114f, 0.114f, 0.114f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.239f, 0.239f, 0.239f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.188f, 0.188f, 0.188f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.114f, 0.114f, 0.114f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.188f, 0.188f, 0.188f, 1.00f);

    style.WindowRounding    = 0.0f;
    style.ChildRounding     = NXN_S(4.0f);
    style.FrameRounding     = NXN_S(4.0f);
    style.GrabRounding      = NXN_S(3.0f);
    style.PopupRounding     = NXN_S(4.0f);
    style.ScrollbarRounding = NXN_S(4.0f);
    style.TabRounding       = NXN_S(4.0f);
    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.ItemSpacing       = ImVec2(NXN_S(5.0f), NXN_S(4.0f));
    style.FramePadding      = ImVec2(NXN_S(6.0f), NXN_S(3.0f));
    style.WindowPadding     = ImVec2(NXN_S(6.0f), NXN_S(5.0f));
}

} // namespace BlenderTheme
