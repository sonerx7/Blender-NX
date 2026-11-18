#pragma once
#include "imgui.h"
#include "Scene.h"
#include <cstdio>
#include <cmath>

namespace BlenderWidgets {

// ----------------------------------------------------------------------
// Vector Blender 4.2 Logo (Orange 3-arm ring + White eye + Blue pupil)
// ----------------------------------------------------------------------
inline void drawBlenderLogo(ImDrawList* dl, ImVec2 center, float r) {
    ImU32 colOrange = IM_COL32(232, 125, 13, 255);
    ImU32 colWhite  = IM_COL32(245, 245, 248, 255);
    ImU32 colBlue   = IM_COL32(38, 120, 210, 255);

    // 3 iconic arms
    dl->AddLine(ImVec2(center.x - r * 0.95f, center.y - r * 0.15f), ImVec2(center.x - r * 0.25f, center.y - r * 0.15f), colOrange, r * 0.26f);
    dl->AddLine(ImVec2(center.x - r * 0.65f, center.y - r * 0.68f), ImVec2(center.x + r * 0.15f, center.y - r * 0.25f), colOrange, r * 0.26f);
    dl->AddLine(ImVec2(center.x - r * 0.72f, center.y + r * 0.55f), ImVec2(center.x - r * 0.15f, center.y + r * 0.12f), colOrange, r * 0.26f);

    // Outer orange ring + inner white + blue center
    dl->AddCircleFilled(ImVec2(center.x + r * 0.12f, center.y + r * 0.05f), r * 0.62f, colOrange, 20);
    dl->AddCircleFilled(ImVec2(center.x + r * 0.12f, center.y + r * 0.05f), r * 0.36f, colWhite, 20);
    dl->AddCircleFilled(ImVec2(center.x + r * 0.12f, center.y + r * 0.05f), r * 0.20f, colBlue, 16);
}

// ----------------------------------------------------------------------
// Stacked Blender-Style Transform XYZ (No clipping, full row width)
// ----------------------------------------------------------------------
inline void drawTransformXYZ(const char* label, Vec3& v, float speed = 0.05f, const char* unit = "m") {
    ImGui::PushID(label);
    ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.77f, 1.0f), "%s", label);

    float availW = ImGui::GetContentRegionAvail().x;
    float axisW = NXN_S(18.0f);
    float inputW = availW - axisW - NXN_S(6.0f);
    if (inputW < NXN_S(60.0f)) inputW = NXN_S(60.0f);

    auto drawAxisRow = [&](const char* axis, float& val, ImVec4 axisCol, const char* id) {
        ImGui::PushStyleColor(ImGuiCol_Button, axisCol);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, axisCol);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, axisCol);
        ImGui::Button(axis, ImVec2(axisW, NXN_S(20.0f)));
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, NXN_S(2.0f));
        ImGui::SetNextItemWidth(inputW);
        char fmt[32];
        snprintf(fmt, sizeof fmt, "%%0.3f %s", unit);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(NXN_S(6.0f), NXN_S(2.0f)));
        ImGui::DragFloat(id, &val, speed, 0.0f, 0.0f, fmt);
        ImGui::PopStyleVar();
    };

    drawAxisRow("X", v.x, ImVec4(0.65f, 0.22f, 0.22f, 1.0f), "##X");
    drawAxisRow("Y", v.y, ImVec4(0.22f, 0.55f, 0.22f, 1.0f), "##Y");
    drawAxisRow("Z", v.z, ImVec4(0.22f, 0.35f, 0.65f, 1.0f), "##Z");

    ImGui::PopID();
}

// ----------------------------------------------------------------------
// Clean Blender Property Sliders (Never clipped on narrow sidebars)
// ----------------------------------------------------------------------
inline void drawSliderProperty(const char* label, float& val, float minVal, float maxVal, const char* fmt = "%0.3f") {
    ImGui::PushID(label);
    float availW = ImGui::GetContentRegionAvail().x;
    float labelW = availW * 0.45f;
    if (labelW < NXN_S(70.0f)) labelW = NXN_S(70.0f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.78f, 0.78f, 0.80f, 1.0f), "%s", label);
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(availW - labelW);
    ImGui::SliderFloat("##val", &val, minVal, maxVal, fmt);
    ImGui::PopID();
}

inline void drawIntSliderProperty(const char* label, int& val, int minVal, int maxVal) {
    ImGui::PushID(label);
    float availW = ImGui::GetContentRegionAvail().x;
    float labelW = availW * 0.48f;
    if (labelW < NXN_S(80.0f)) labelW = NXN_S(80.0f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.78f, 0.78f, 0.80f, 1.0f), "%s", label);
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(availW - labelW);
    ImGui::SliderInt("##ival", &val, minVal, maxVal);
    ImGui::PopID();
}

inline void drawColorProperty(const char* label, Vec4& col) {
    ImGui::PushID(label);
    float availW = ImGui::GetContentRegionAvail().x;
    float labelW = availW * 0.45f;
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.78f, 0.78f, 0.80f, 1.0f), "%s", label);
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(availW - labelW);
    float c[4] = {col.x, col.y, col.z, col.w};
    if (ImGui::ColorEdit4("##col", c, ImGuiColorEditFlags_NoInputs)) {
        col = Vec4(c[0], c[1], c[2], c[3]);
    }
    ImGui::PopID();
}

// ----------------------------------------------------------------------
// Blender Top Workspace Tab (Transparent idle, #303030 active pill)
// ----------------------------------------------------------------------
inline bool drawWorkspaceTab(const char* name, bool active) {
    ImVec4 activeBg   = ImVec4(0.188f, 0.188f, 0.188f, 1.0f); // #303030
    ImVec4 inactiveBg = ImVec4(0.114f, 0.114f, 0.114f, 0.0f); // Transparent
    ImVec4 hoverBg    = ImVec4(0.220f, 0.220f, 0.225f, 1.0f);
    ImVec4 textColor  = active ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.68f, 0.68f, 0.70f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, active ? activeBg : inactiveBg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active ? activeBg : hoverBg);
    ImGui::PushStyleColor(ImGuiCol_Text, textColor);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, NXN_S(4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(NXN_S(8.0f), NXN_S(3.0f)));

    bool clicked = ImGui::Button(name);

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    return clicked;
}

// ----------------------------------------------------------------------
// Top Menu Item (Flat transparent Blender style)
// ----------------------------------------------------------------------
inline bool drawMenuButton(const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.24f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.86f, 0.86f, 0.88f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(NXN_S(6.0f), NXN_S(3.0f)));
    bool clicked = ImGui::Button(label);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    return clicked;
}

// ----------------------------------------------------------------------
// Vector Icons for Left Toolbar (T-Panel)
// ----------------------------------------------------------------------
enum ToolIconType {
    ICON_SELECT = 0, ICON_CURSOR, ICON_MOVE, ICON_ROTATE, ICON_SCALE,
    ICON_EXTRUDE, ICON_INSET, ICON_SUBDIVIDE,
    ICON_SCULPT_DRAW, ICON_SCULPT_CLAY, ICON_SCULPT_SMOOTH,
    ICON_SCULPT_FLATTEN, ICON_SCULPT_INFLATE, ICON_SCULPT_GRAB,
    ICON_SYMMETRY_X
};

inline bool drawToolIconButton(const char* id, ToolIconType icon, bool active, const char* tooltip, float sz) {
    ImGui::PushID(id);
    ImVec4 bgCol = active ? ImVec4(0.278f, 0.447f, 0.702f, 1.0f) : ImVec4(0.220f, 0.220f, 0.225f, 1.0f);
    ImVec4 hovCol = active ? ImVec4(0.320f, 0.500f, 0.780f, 1.0f) : ImVec4(0.300f, 0.300f, 0.310f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, bgCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovCol);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, NXN_S(4.0f));

    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::Button("##tool", ImVec2(sz, sz));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 c(p.x + sz * 0.5f, p.y + sz * 0.5f);
    float r = sz * 0.30f;

    ImU32 white = IM_COL32(235, 235, 240, 255);
    ImU32 red   = IM_COL32(230, 80, 80, 255);
    ImU32 green = IM_COL32(90, 215, 90, 255);
    ImU32 blue  = IM_COL32(85, 155, 245, 255);
    ImU32 orange= IM_COL32(235, 140, 40, 255);

    switch (icon) {
        case ICON_SELECT: {
            // Dashed box + cursor
            dl->AddRect(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r * 0.6f, c.y + r * 0.6f), white, 0, 0, 1.4f);
            dl->AddTriangleFilled(ImVec2(c.x, c.y), ImVec2(c.x + r * 0.9f, c.y + r * 0.45f), ImVec2(c.x + r * 0.45f, c.y + r * 0.9f), orange);
            break;
        }
        case ICON_CURSOR: {
            dl->AddCircle(c, r * 0.7f, white, 16, 1.5f);
            dl->AddCircle(c, r * 0.7f, red, 8, 1.5f);
            dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), white, 1.3f);
            dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y + r), white, 1.3f);
            break;
        }
        case ICON_MOVE: {
            dl->AddLine(c, ImVec2(c.x + r, c.y + r * 0.35f), red, 2.0f);
            dl->AddLine(c, ImVec2(c.x - r * 0.7f, c.y + r * 0.5f), green, 2.0f);
            dl->AddLine(c, ImVec2(c.x, c.y - r), blue, 2.0f);
            dl->AddCircleFilled(c, NXN_S(2.5f), white);
            break;
        }
        case ICON_ROTATE: {
            dl->AddCircle(c, r * 0.85f, blue, 20, 1.8f);
            dl->AddCircle(c, r * 0.55f, red, 16, 1.5f);
            dl->AddCircleFilled(ImVec2(c.x + r * 0.85f, c.y), NXN_S(2.2f), white);
            break;
        }
        case ICON_SCALE: {
            dl->AddRect(ImVec2(c.x - r * 0.85f, c.y - r * 0.2f), ImVec2(c.x - r * 0.1f, c.y + r * 0.85f), white, 0, 0, 1.5f);
            dl->AddRectFilled(ImVec2(c.x, c.y - r * 0.85f), ImVec2(c.x + r * 0.85f, c.y), green);
            dl->AddLine(ImVec2(c.x - r * 0.3f, c.y + r * 0.3f), ImVec2(c.x + r * 0.4f, c.y - r * 0.4f), orange, 1.8f);
            break;
        }
        case ICON_EXTRUDE: {
            dl->AddRect(ImVec2(c.x - r * 0.7f, c.y - r * 0.1f), ImVec2(c.x + r * 0.7f, c.y + r * 0.85f), white, 0, 0, 1.5f);
            dl->AddRectFilled(ImVec2(c.x - r * 0.5f, c.y - r * 0.85f), ImVec2(c.x + r * 0.5f, c.y - r * 0.25f), orange);
            dl->AddLine(ImVec2(c.x, c.y + r * 0.1f), ImVec2(c.x, c.y - r * 0.5f), white, 1.6f);
            break;
        }
        case ICON_INSET: {
            dl->AddRect(ImVec2(c.x - r * 0.85f, c.y - r * 0.85f), ImVec2(c.x + r * 0.85f, c.y + r * 0.85f), white, 0, 0, 1.4f);
            dl->AddRectFilled(ImVec2(c.x - r * 0.4f, c.y - r * 0.4f), ImVec2(c.x + r * 0.4f, c.y + r * 0.4f), orange);
            break;
        }
        case ICON_SUBDIVIDE: {
            dl->AddRect(ImVec2(c.x - r * 0.85f, c.y - r * 0.85f), ImVec2(c.x + r * 0.85f, c.y + r * 0.85f), white, 0, 0, 1.4f);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.85f), ImVec2(c.x, c.y + r * 0.85f), orange, 1.4f);
            dl->AddLine(ImVec2(c.x - r * 0.85f, c.y), ImVec2(c.x + r * 0.85f, c.y), orange, 1.4f);
            break;
        }
        case ICON_SCULPT_DRAW: {
            dl->AddCircleFilled(c, r * 0.75f, IM_COL32(205, 85, 65, 255), 16);
            dl->AddCircleFilled(ImVec2(c.x - r * 0.2f, c.y - r * 0.2f), r * 0.25f, white, 12);
            break;
        }
        case ICON_SCULPT_CLAY: {
            dl->AddRectFilled(ImVec2(c.x - r * 0.8f, c.y - r * 0.4f), ImVec2(c.x + r * 0.8f, c.y + r * 0.4f), IM_COL32(215, 110, 75, 255), NXN_S(3.0f));
            dl->AddLine(ImVec2(c.x - r * 0.6f, c.y), ImVec2(c.x + r * 0.6f, c.y), white, 1.5f);
            break;
        }
        case ICON_SCULPT_SMOOTH: {
            dl->AddBezierCubic(ImVec2(c.x - r, c.y + r * 0.4f), ImVec2(c.x - r * 0.3f, c.y - r * 0.8f),
                               ImVec2(c.x + r * 0.3f, c.y + r * 0.8f), ImVec2(c.x + r, c.y - r * 0.4f), blue, 2.2f);
            break;
        }
        case ICON_SCULPT_FLATTEN: {
            dl->AddLine(ImVec2(c.x - r, c.y + r * 0.3f), ImVec2(c.x + r, c.y + r * 0.3f), white, 2.2f);
            dl->AddTriangleFilled(ImVec2(c.x, c.y + r * 0.1f), ImVec2(c.x - r * 0.4f, c.y - r * 0.6f), ImVec2(c.x + r * 0.4f, c.y - r * 0.6f), orange);
            break;
        }
        case ICON_SCULPT_INFLATE: {
            dl->AddCircle(c, r * 0.85f, orange, 18, 2.0f);
            dl->AddCircleFilled(c, r * 0.45f, IM_COL32(235, 160, 80, 200), 14);
            break;
        }
        case ICON_SCULPT_GRAB: {
            dl->AddCircle(c, r * 0.45f, white, 12, 1.5f);
            dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), orange, 1.8f);
            dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y + r), orange, 1.8f);
            break;
        }
        case ICON_SYMMETRY_X: {
            dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y + r), red, 1.8f);
            dl->AddTriangleFilled(ImVec2(c.x - r * 0.25f, c.y), ImVec2(c.x - r * 0.85f, c.y - r * 0.45f), ImVec2(c.x - r * 0.85f, c.y + r * 0.45f), white);
            dl->AddTriangleFilled(ImVec2(c.x + r * 0.25f, c.y), ImVec2(c.x + r * 0.85f, c.y - r * 0.45f), ImVec2(c.x + r * 0.85f, c.y + r * 0.45f), white);
            break;
        }
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    ImGui::PopID();
    return clicked;
}

// ----------------------------------------------------------------------
// Properties Sidebar Vertical Vector Icons
// ----------------------------------------------------------------------
inline bool drawPropVerticalTab(const char* id, int tabIdx, bool active, const char* tooltip) {
    ImGui::PushID(id);
    ImVec4 bgCol = active ? ImVec4(0.188f, 0.188f, 0.188f, 1.0f) : ImVec4(0.125f, 0.125f, 0.128f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, bgCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.24f, 0.25f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, NXN_S(3.0f));

    float sz = NXN_S(24.0f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::Button("##ptab", ImVec2(sz, sz));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (active) {
        // Active left orange/blue bar indicator
        dl->AddRectFilled(p, ImVec2(p.x + NXN_S(2.5f), p.y + sz), IM_COL32(71, 114, 179, 255));
    }

    ImVec2 c(p.x + sz * 0.5f, p.y + sz * 0.5f);
    float r = sz * 0.28f;
    ImU32 col = active ? IM_COL32(255, 255, 255, 255) : IM_COL32(165, 165, 170, 255);

    if (tabIdx == 0) { // Tool (Wrench + Screwdriver)
        dl->AddLine(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), col, 1.8f);
        dl->AddLine(ImVec2(c.x + r, c.y - r), ImVec2(c.x - r, c.y + r), col, 1.8f);
    } else if (tabIdx == 1) { // Render (Camera)
        dl->AddRect(ImVec2(c.x - r, c.y - r * 0.7f), ImVec2(c.x + r, c.y + r * 0.7f), col, 1.0f, 0, 1.5f);
        dl->AddCircleFilled(c, r * 0.35f, col, 12);
    } else if (tabIdx == 2) { // World (Globe)
        dl->AddCircle(c, r * 0.9f, IM_COL32(220, 100, 100, 255), 16, 1.5f);
        dl->AddLine(ImVec2(c.x - r * 0.9f, c.y), ImVec2(c.x + r * 0.9f, c.y), IM_COL32(220, 100, 100, 255), 1.0f);
        dl->AddLine(ImVec2(c.x, c.y - r * 0.9f), ImVec2(c.x, c.y + r * 0.9f), IM_COL32(220, 100, 100, 255), 1.0f);
    } else if (tabIdx == 3) { // Object (Orange Cube)
        dl->AddRect(ImVec2(c.x - r * 0.8f, c.y - r * 0.8f), ImVec2(c.x + r * 0.8f, c.y + r * 0.8f), IM_COL32(235, 145, 45, 255), 0, 0, 1.6f);
        dl->AddCircleFilled(c, NXN_S(2.0f), IM_COL32(235, 145, 45, 255));
    } else if (tabIdx == 4) { // Modifiers (Blue Wrench)
        dl->AddLine(ImVec2(c.x - r * 0.8f, c.y + r * 0.8f), ImVec2(c.x + r * 0.6f, c.y - r * 0.6f), IM_COL32(95, 165, 245, 255), 2.4f);
        dl->AddCircle(ImVec2(c.x + r * 0.6f, c.y - r * 0.6f), r * 0.4f, IM_COL32(95, 165, 245, 255), 12, 1.8f);
    } else if (tabIdx == 5) { // Material (Red/White Sphere)
        dl->AddCircleFilled(c, r * 0.85f, IM_COL32(220, 75, 75, 255), 16);
        dl->AddCircleFilled(ImVec2(c.x - r * 0.25f, c.y - r * 0.25f), r * 0.3f, IM_COL32(245, 245, 245, 220), 12);
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    ImGui::PopID();
    return clicked;
}

// ----------------------------------------------------------------------
// Outliner Vector Icons (Mesh / Light / Camera / Eye)
// ----------------------------------------------------------------------
inline void drawOutlinerObjectIcon(ImDrawList* dl, ImVec2 pos, ObjectType type, bool selected) {
    float r = NXN_S(5.0f);
    ImVec2 c(pos.x + r, pos.y + NXN_S(7.0f));
    if (type == OBJ_MESH) {
        ImU32 col = selected ? IM_COL32(240, 145, 45, 255) : IM_COL32(105, 195, 130, 255);
        dl->AddTriangle(ImVec2(c.x - r, c.y - r * 0.6f), ImVec2(c.x + r, c.y - r * 0.6f), ImVec2(c.x, c.y + r * 0.8f), col, 1.5f);
    } else if (type == OBJ_LIGHT) {
        dl->AddCircleFilled(c, r * 0.7f, IM_COL32(245, 210, 65, 255), 12);
        dl->AddCircle(c, r * 1.05f, IM_COL32(245, 210, 65, 180), 12, 1.0f);
    } else if (type == OBJ_CAMERA) {
        dl->AddRectFilled(ImVec2(c.x - r * 0.8f, c.y - r * 0.5f), ImVec2(c.x + r * 0.3f, c.y + r * 0.5f), IM_COL32(105, 185, 115, 255));
        dl->AddTriangleFilled(ImVec2(c.x + r * 0.3f, c.y), ImVec2(c.x + r, c.y - r * 0.5f), ImVec2(c.x + r, c.y + r * 0.5f), IM_COL32(105, 185, 115, 255));
    }
}

inline void drawEyeIcon(ImDrawList* dl, ImVec2 c, bool visible) {
    float r = NXN_S(5.5f);
    ImU32 col = visible ? IM_COL32(215, 215, 220, 255) : IM_COL32(95, 95, 100, 180);
    dl->AddEllipse(c, ImVec2(r, r * 0.55f), col, 0.0f, 14, 1.3f);
    if (visible) {
        dl->AddCircleFilled(c, r * 0.32f, col, 10);
    } else {
        dl->AddLine(ImVec2(c.x - r, c.y + r * 0.6f), ImVec2(c.x + r, c.y - r * 0.6f), IM_COL32(190, 80, 80, 220), 1.4f);
    }
}

// ----------------------------------------------------------------------
// Realistic 3D Shaded Viewport Mode Spheres
// ----------------------------------------------------------------------
inline void drawShadingSpheres(Scene& scene) {
    auto drawSphereBtn = [&](const char* id, ShadingMode mode, ImVec4 sphereCol, const char* tooltip) {
        bool selected = (scene.shadingMode == mode);
        ImGui::PushID(id);
        ImVec4 btnBg = selected ? ImVec4(0.278f, 0.447f, 0.702f, 1.0f) : ImVec4(0.18f, 0.18f, 0.19f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, btnBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selected ? btnBg : ImVec4(0.28f, 0.28f, 0.30f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, NXN_S(4.0f));

        ImVec2 p = ImGui::GetCursorScreenPos();
        float sz = NXN_S(22.0f);
        if (ImGui::Button("##btn", ImVec2(sz, sz))) {
            scene.shadingMode = mode;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 center(p.x + sz * 0.5f, p.y + sz * 0.5f);
        float r = sz * 0.36f;

        if (mode == SHADE_WIREFRAME) {
            dl->AddCircle(center, r, IM_COL32(220, 220, 225, 255), 16, 1.4f);
            dl->AddEllipse(center, ImVec2(r * 0.48f, r), IM_COL32(180, 180, 185, 220), 0.0f, 14, 1.0f);
            dl->AddLine(ImVec2(center.x - r, center.y), ImVec2(center.x + r, center.y), IM_COL32(180, 180, 185, 220), 1.0f);
        } else if (mode == SHADE_SOLID) {
            dl->AddCircleFilled(center, r, IM_COL32(185, 185, 190, 255), 16);
            dl->AddCircleFilled(ImVec2(center.x - r * 0.28f, center.y - r * 0.28f), r * 0.32f, IM_COL32(245, 245, 250, 230), 12);
        } else if (mode == SHADE_MATERIAL) {
            dl->AddCircleFilled(center, r, IM_COL32(210, 110, 65, 255), 16);
            dl->AddCircleFilled(ImVec2(center.x - r * 0.3f, center.y - r * 0.3f), r * 0.35f, IM_COL32(245, 215, 180, 220), 12);
            dl->AddCircle(center, r, IM_COL32(240, 240, 240, 150), 16, 1.0f);
        } else { // SHADE_RENDERED
            dl->AddCircleFilled(center, r, IM_COL32(235, 195, 70, 255), 16);
            dl->AddCircleFilled(ImVec2(center.x - r * 0.28f, center.y - r * 0.28f), r * 0.3f, IM_COL32(255, 255, 240, 240), 12);
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        ImGui::PopID();
    };

    drawSphereBtn("Wire", SHADE_WIREFRAME, ImVec4(0.3f, 0.3f, 0.3f, 1.0f), "Wireframe");
    ImGui::SameLine(0, NXN_S(2.0f));
    drawSphereBtn("Solid", SHADE_SOLID, ImVec4(0.75f, 0.75f, 0.78f, 1.0f), "Solid (Studio/MatCap)");
    ImGui::SameLine(0, NXN_S(2.0f));
    drawSphereBtn("Mat", SHADE_MATERIAL, ImVec4(0.88f, 0.45f, 0.20f, 1.0f), "Material Preview (Principled BSDF v2)");
    ImGui::SameLine(0, NXN_S(2.0f));
    drawSphereBtn("Render", SHADE_RENDERED, ImVec4(0.95f, 0.80f, 0.25f, 1.0f), "Rendered (EEVEE Next)");

    // Shading Options Dropdown
    ImGui::SameLine(0, NXN_S(2.0f));
    if (ImGui::Button("v##ShadingMenu", ImVec2(NXN_S(18.0f), NXN_S(22.0f)))) {
        ImGui::OpenPopup("ShadingOptionsPopup");
    }
    if (ImGui::BeginPopup("ShadingOptionsPopup")) {
        ImGui::TextColored(ImVec4(0.92f, 0.47f, 0.05f, 1.0f), "Viewport Shading");
        ImGui::Separator();

        ImGui::TextDisabled("Lighting:");
        if (ImGui::RadioButton("Studio", (int)scene.solidLighting == (int)LIGHT_STUDIO)) scene.solidLighting = LIGHT_STUDIO;
        ImGui::SameLine();
        if (ImGui::RadioButton("MatCap", (int)scene.solidLighting == (int)LIGHT_MATCAP)) scene.solidLighting = LIGHT_MATCAP;
        ImGui::SameLine();
        if (ImGui::RadioButton("Flat", (int)scene.solidLighting == (int)LIGHT_FLAT)) scene.solidLighting = LIGHT_FLAT;

        if (scene.solidLighting == LIGHT_MATCAP) {
            ImGui::Separator();
            ImGui::TextDisabled("MatCap Preset:");
            if (ImGui::Selectable("Clay Terracotta", scene.matcapType == MATCAP_CLAY)) scene.matcapType = MATCAP_CLAY;
            if (ImGui::Selectable("Red Wax (ZBrush/Sculpt)", scene.matcapType == MATCAP_RED_WAX)) scene.matcapType = MATCAP_RED_WAX;
            if (ImGui::Selectable("Chrome Horizon", scene.matcapType == MATCAP_CHROME)) scene.matcapType = MATCAP_CHROME;
            if (ImGui::Selectable("Pearl Sheen", scene.matcapType == MATCAP_PEARL)) scene.matcapType = MATCAP_PEARL;
            if (ImGui::Selectable("Normal Check", scene.matcapType == MATCAP_NORMAL)) scene.matcapType = MATCAP_NORMAL;
        }

        ImGui::Separator();
        ImGui::Checkbox("Cavity / Ridge Accents", &scene.showCavity);
        ImGui::Checkbox("X-Ray Transparency (Alt+Z)", &scene.showXRay);
        ImGui::Checkbox("Wireframe Overlay", &scene.showWireOverlay);

        ImGui::EndPopup();
    }
}

// ----------------------------------------------------------------------
// Nexvyn OS Joy-Con / Controller Pill Badge for Status Bar
// ----------------------------------------------------------------------
inline void drawControlHint(const char* keyBadge, const char* action) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.24f, 0.26f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.98f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, NXN_S(3.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(NXN_S(4.0f), NXN_S(1.0f)));
    ImGui::SmallButton(keyBadge);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    ImGui::SameLine(0, NXN_S(3.0f));
    ImGui::TextDisabled("%s", action);
    ImGui::SameLine(0, NXN_S(8.0f));
}

} // namespace BlenderWidgets
