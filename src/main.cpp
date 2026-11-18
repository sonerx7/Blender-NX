// Nexvyn Blender (Blender 4.2 LTS NX)
// Authentic 1:1 Blender 4.2 LTS UI & 3D Creation Suite for Nexvyn OS N3-N4 High-Performance Execution
#include <glad/glad.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>

#include <nexvyn/nexvyn_native.h>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include <nexvyn/nxn_os_ui.h>

#include "Scene.h"
#include "Primitives.h"
#include "BlenderTheme.h"
#include "BlenderWidgets.h"
#include "Viewport3D.h"
#include "BlenderIO.h"
#include "MeshOps.h"

static nxn_display* g_disp = nullptr;
static uint32_t g_w = 1280, g_h = 720;
static nxn_ui_tokens g_tok = NXN_STRUCT_INIT(nxn_ui_tokens);
static nxn_input_state g_in = NXN_STRUCT_INIT(nxn_input_state);

static Scene g_scene;
static Viewport3D g_viewport;
static std::string g_statusMsg = "Blender 4.2 LTS NX - N3/N4 Native Execution Ready";
static bool g_showSplashModal = true;
static bool g_showAboutModal = false;
static bool g_showTelemetryModal = false;
static bool g_exportSuccessModal = false;
static std::string g_lastExportPath = "";
static char g_outlinerFilter[64] = "";

enum PropertiesTab {
    TAB_TOOL,
    TAB_RENDER,
    TAB_WORLD,
    TAB_OBJECT,
    TAB_MODIFIERS,
    TAB_MATERIAL
};
static PropertiesTab g_curPropTab = TAB_OBJECT;
static int g_activeWorkspace = 0; // 0=Layout, 1=Modeling, 2=Sculpting, 3=UV Editing, 4=Shading, 5=Animation, 6=GeoNodes
static int g_activeTool = 0;      // 0=Select, 1=Cursor, 2=Move, 3=Rotate, 4=Scale

// N3-N4 Telemetry counters
static float g_fps = 60.0f;
static float g_frameTimeMs = 16.6f;
static uint64_t g_totalFramesRendered = 0;

static void switch_workspace(int i) {
    g_activeWorkspace = ((i % 7) + 7) % 7;
    if (g_activeWorkspace == 0) { // Layout
        g_scene.editMode = MODE_OBJECT;
        g_scene.shadingMode = SHADE_SOLID;
        g_scene.solidLighting = LIGHT_STUDIO;
        g_scene.isPlaying = false;
    } else if (g_activeWorkspace == 1) { // Modeling
        g_scene.editMode = MODE_EDIT_VERTEX;
        g_scene.shadingMode = SHADE_SOLID;
        g_scene.isPlaying = false;
    } else if (g_activeWorkspace == 2) { // Sculpting
        g_scene.editMode = MODE_SCULPT;
        g_scene.shadingMode = SHADE_SOLID;
        g_scene.solidLighting = LIGHT_MATCAP;
        g_scene.matcapType = MATCAP_RED_WAX;
        g_scene.isPlaying = false;
    } else if (g_activeWorkspace == 3) { // UV Edit
        g_scene.editMode = MODE_EDIT_FACE;
        g_scene.shadingMode = SHADE_SOLID;
        g_scene.isPlaying = false;
    } else if (g_activeWorkspace == 4) { // Shading
        g_scene.editMode = MODE_OBJECT;
        g_scene.shadingMode = SHADE_MATERIAL;
        g_curPropTab = TAB_MATERIAL;
        g_scene.isPlaying = false;
    } else if (g_activeWorkspace == 5) { // Animation
        g_scene.editMode = MODE_OBJECT;
        g_scene.shadingMode = SHADE_SOLID;
        g_scene.isPlaying = true;
    } else if (g_activeWorkspace == 6) { // GeoNodes
        g_scene.editMode = MODE_OBJECT;
        g_scene.shadingMode = SHADE_SOLID;
        g_curPropTab = TAB_MODIFIERS;
        g_scene.isPlaying = false;
    }
}

static void init_default_scene() {
    g_scene.name = "Scene";
    g_scene.objects.clear();

    // 1. Default Blender Cube
    SceneObject cube;
    cube.name = "Cube";
    cube.type = OBJ_MESH;
    cube.location = Vec3(-1.8f, 0.0f, 0.0f);
    cube.rotation = Vec3(0, 0, 0);
    cube.scale = Vec3(1, 1, 1);
    cube.mesh = Primitives::createCube(2.0f);
    cube.material.name = "CubeMat";
    cube.material.baseColor = Vec4(0.75f, 0.78f, 0.82f, 1.0f);
    cube.selected = false;
    cube.visible = true;
    cube.renderVisible = true;
    cube.selectable = true;
    g_scene.objects.push_back(cube);

    // 2. Iconic Suzanne the Monkey (Blender 4.2 Mascot)
    SceneObject monk;
    monk.name = "Suzanne";
    monk.type = OBJ_MESH;
    monk.location = Vec3(1.8f, 0.5f, 0.0f);
    monk.rotation = Vec3(0, -20.0f, 0);
    monk.scale = Vec3(1.2f, 1.2f, 1.2f);
    monk.mesh = Primitives::createSuzanne();
    monk.material.name = "SuzanneMat";
    monk.material.baseColor = Vec4(0.92f, 0.62f, 0.38f, 1.0f);
    monk.material.roughness = 0.35f;
    monk.material.specular = 0.65f;
    monk.selected = true;
    monk.visible = true;
    monk.renderVisible = true;
    monk.selectable = true;
    monk.modifiers.push_back({"Subdivision", MOD_SUBSURF, true, 1, false, false, false, 0.1f, 2, 0.05f});
    g_scene.objects.push_back(monk);

    // 3. Default Lamp (Point Light)
    SceneObject light;
    light.name = "Point Light";
    light.type = OBJ_LIGHT;
    light.location = Vec3(4.0f, 5.0f, -2.0f);
    light.mesh = Primitives::createLightWireframe();
    light.visible = true;
    light.renderVisible = true;
    light.selectable = true;
    g_scene.objects.push_back(light);

    // 4. Default Camera
    SceneObject cam;
    cam.name = "Camera";
    cam.type = OBJ_CAMERA;
    cam.location = Vec3(0.0f, 3.5f, 6.0f);
    cam.rotation = Vec3(-25.0f, 0.0f, 0.0f);
    cam.mesh = Primitives::createCameraWireframe();
    cam.visible = true;
    cam.renderVisible = true;
    cam.selectable = true;
    g_scene.objects.push_back(cam);

    g_scene.selectedIndex = 1; // Suzanne is active!
}

static void add_suzanne() {
    SceneObject obj;
    obj.name = "Suzanne";
    obj.type = OBJ_MESH;
    obj.location = g_scene.cursor3D;
    obj.scale = Vec3(1, 1, 1);
    obj.mesh = Primitives::createSuzanne();
    obj.material.name = "MonkeyMat";
    obj.material.baseColor = Vec4(0.92f, 0.62f, 0.38f, 1.0f);
    g_scene.objects.push_back(obj);
    g_scene.selectObject((int)g_scene.objects.size() - 1);
    g_statusMsg = "Added Monkey (Suzanne) to Scene";
}

static void add_cube() {
    SceneObject obj;
    obj.name = "Cube";
    obj.type = OBJ_MESH;
    obj.location = g_scene.cursor3D;
    obj.mesh = Primitives::createCube(2.0f);
    g_scene.objects.push_back(obj);
    g_scene.selectObject((int)g_scene.objects.size() - 1);
    g_statusMsg = "Added Cube to Scene";
}

static void add_uv_sphere() {
    SceneObject obj;
    obj.name = "UV Sphere";
    obj.type = OBJ_MESH;
    obj.location = g_scene.cursor3D;
    obj.mesh = Primitives::createUVSphere(1.0f, 16, 24);
    obj.material.baseColor = Vec4(0.35f, 0.65f, 0.95f, 1.0f);
    g_scene.objects.push_back(obj);
    g_scene.selectObject((int)g_scene.objects.size() - 1);
    g_statusMsg = "Added UV Sphere to Scene";
}

static void add_ico_sphere() {
    SceneObject obj;
    obj.name = "Ico Sphere";
    obj.type = OBJ_MESH;
    obj.location = g_scene.cursor3D;
    obj.mesh = Primitives::createIcoSphere(1.0f);
    obj.material.baseColor = Vec4(0.40f, 0.85f, 0.80f, 1.0f);
    g_scene.objects.push_back(obj);
    g_scene.selectObject((int)g_scene.objects.size() - 1);
    g_statusMsg = "Added Ico Sphere to Scene";
}

static void add_cylinder() {
    SceneObject obj;
    obj.name = "Cylinder";
    obj.type = OBJ_MESH;
    obj.location = g_scene.cursor3D;
    obj.mesh = Primitives::createCylinder(1.0f, 2.0f, 16);
    obj.material.baseColor = Vec4(0.40f, 0.85f, 0.50f, 1.0f);
    g_scene.objects.push_back(obj);
    g_scene.selectObject((int)g_scene.objects.size() - 1);
    g_statusMsg = "Added Cylinder to Scene";
}

static void add_cone() {
    SceneObject obj;
    obj.name = "Cone";
    obj.type = OBJ_MESH;
    obj.location = g_scene.cursor3D;
    obj.mesh = Primitives::createCone(1.0f, 2.0f, 16);
    obj.material.baseColor = Vec4(0.95f, 0.85f, 0.30f, 1.0f);
    g_scene.objects.push_back(obj);
    g_scene.selectObject((int)g_scene.objects.size() - 1);
    g_statusMsg = "Added Cone to Scene";
}

static void add_torus() {
    SceneObject obj;
    obj.name = "Torus";
    obj.type = OBJ_MESH;
    obj.location = g_scene.cursor3D;
    obj.mesh = Primitives::createTorus(1.2f, 0.35f, 20, 12);
    obj.material.baseColor = Vec4(0.85f, 0.40f, 0.75f, 1.0f);
    g_scene.objects.push_back(obj);
    g_scene.selectObject((int)g_scene.objects.size() - 1);
    g_statusMsg = "Added Torus to Scene";
}

static void add_plane() {
    SceneObject obj;
    obj.name = "Plane";
    obj.type = OBJ_MESH;
    obj.location = g_scene.cursor3D;
    obj.mesh = Primitives::createPlane(6.0f);
    g_scene.objects.push_back(obj);
    g_scene.selectObject((int)g_scene.objects.size() - 1);
    g_statusMsg = "Added Plane to Scene";
}

static void delete_selected() {
    if (g_scene.selectedIndex >= 0 && g_scene.selectedIndex < (int)g_scene.objects.size()) {
        std::string delName = g_scene.objects[g_scene.selectedIndex].name;
        g_scene.objects.erase(g_scene.objects.begin() + g_scene.selectedIndex);
        if (g_scene.objects.empty()) {
            g_scene.selectedIndex = -1;
        } else {
            g_scene.selectedIndex = 0;
            g_scene.objects[0].selected = true;
        }
        g_statusMsg = "Deleted object: " + delName;
    }
}

static void toggle_mode() {
    if (g_scene.editMode == MODE_OBJECT) {
        g_scene.editMode = MODE_EDIT_VERTEX;
        g_statusMsg = "Mode: Edit Mode (Vertices)";
    } else if (g_scene.editMode == MODE_EDIT_VERTEX) {
        g_scene.editMode = MODE_SCULPT;
        g_statusMsg = "Mode: Sculpt Mode";
    } else {
        g_scene.editMode = MODE_OBJECT;
        g_statusMsg = "Mode: Object Mode";
    }
}

static void apply_tokens() {
    nxn_ui_tokens_get(&g_tok);
}

static void feed_imgui(float dt) {
    static nxn_imgui_input_state backend;
    nxn_imgui_input(backend, g_in, dt);
}

// ----------------------------------------------------------------------
// 1. BLENDER 4.2 LTS SPLASH SCREEN DIALOG
// ----------------------------------------------------------------------
static void draw_splash_screen() {
    if (g_showSplashModal) {
        ImGui::OpenPopup("Blender 4.2 LTS Splash");
    }

    ImGui::SetNextWindowSize(ImVec2(NXN_S(540.0f), NXN_S(360.0f)), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Blender 4.2 LTS Splash", &g_showSplashModal, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetWindowPos();
        float w = ImGui::GetWindowWidth();

        // Top Header Banner with Blender 4.2 LTS Signature Orange gradient
        dl->AddRectFilled(p0, ImVec2(p0.x + w, p0.y + NXN_S(80.0f)), IM_COL32(235, 110, 15, 255), NXN_S(4.0f), ImDrawFlags_RoundCornersTop);
        dl->AddText(ImVec2(p0.x + NXN_S(20.0f), p0.y + NXN_S(18.0f)), IM_COL32(255, 255, 255, 255), "BLENDER-NX  4.2  LTS");
        dl->AddText(ImVec2(p0.x + NXN_S(20.0f), p0.y + NXN_S(44.0f)), IM_COL32(255, 235, 210, 230), "Port by sonerx7  |  Nexvyn OS N3-N4 Native Execution");

        ImGui::SetCursorPosY(NXN_S(95.0f));

        // Column 1: New File Templates
        ImGui::Columns(2, "SplashCols", false);
        ImGui::SetColumnWidth(0, NXN_S(260.0f));

        ImGui::TextColored(ImVec4(0.92f, 0.47f, 0.05f, 1.0f), "New File");
        ImGui::Separator();
        if (ImGui::Selectable("  [+] General 3D (Default)", false)) {
            init_default_scene();
            g_activeWorkspace = 0;
            g_showSplashModal = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("  [+] 2D Animation", false)) {
            init_default_scene();
            g_scene.shadingMode = SHADE_SOLID;
            g_viewport.setViewFront();
            g_activeWorkspace = 5;
            g_showSplashModal = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("  [+] Sculpting Studio", false)) {
            init_default_scene();
            g_activeWorkspace = 2; // Sculpting
            g_scene.editMode = MODE_SCULPT;
            g_scene.solidLighting = LIGHT_MATCAP;
            g_scene.matcapType = MATCAP_RED_WAX;
            g_showSplashModal = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("  [+] VFX & Simulation", false)) {
            init_default_scene();
            g_activeWorkspace = 4; // Shading
            g_showSplashModal = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("  [+] Video Editing", false)) {
            g_activeWorkspace = 5;
            g_showSplashModal = false;
            ImGui::CloseCurrentPopup();
        }

        // Column 2: Recent Files
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.92f, 0.47f, 0.05f, 1.0f), "Recent Projects");
        ImGui::Separator();
        if (ImGui::Selectable("  Suzanne_Sculpt.blend", false)) {
            init_default_scene();
            g_showSplashModal = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("  Mech_Concept.blend", false)) {
            init_default_scene();
            g_showSplashModal = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("  SciFi_Corridor.blend", false)) {
            init_default_scene();
            g_showSplashModal = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("  Game_Character.obj", false)) {
            init_default_scene();
            g_showSplashModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Columns(1);
        ImGui::Spacing();
        ImGui::Separator();

        // Bottom row
        if (ImGui::Button("Open Project...", ImVec2(NXN_S(120.0f), NXN_S(26.0f)))) {
            g_showSplashModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close Splash", ImVec2(NXN_S(100.0f), NXN_S(26.0f)))) {
            g_showSplashModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Release: 4.2 LTS (Build 2026.11.18)");

        ImGui::EndPopup();
    }
}

// ----------------------------------------------------------------------
// 2. TOP APPLICATION BAR (Menus & Workspace Tabs)
// ----------------------------------------------------------------------
static void draw_top_application_bar(bool& quit) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.114f, 0.114f, 0.114f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(NXN_S(6.0f), NXN_S(3.0f)));
    ImGui::BeginChild("TopApplicationBar", ImVec2(0, NXN_S(28.0f)), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetScrollX(0.0f);

    // Authentic Vector Blender Logo + Version Pill
    ImVec2 logoPos = ImGui::GetCursorScreenPos();
    ImDrawList* topDl = ImGui::GetWindowDrawList();
    BlenderWidgets::drawBlenderLogo(topDl, ImVec2(logoPos.x + NXN_S(12.0f), logoPos.y + NXN_S(11.0f)), NXN_S(9.0f));
    ImGui::Dummy(ImVec2(NXN_S(22.0f), NXN_S(22.0f)));
    ImGui::SameLine(0, NXN_S(4.0f));
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.92f, 0.50f, 0.10f, 1.0f), "4.2");
    ImGui::SameLine(0, NXN_S(6.0f));

    auto drawMenuBtn = [](const char* name, const char* popupId) -> bool {
        bool clicked = BlenderWidgets::drawMenuButton(name);
        if (clicked) ImGui::OpenPopup(popupId);
        return clicked;
    };

    drawMenuBtn("File", "FileMenuPopup");
    ImGui::SameLine(0, NXN_S(2.0f));
    if (ImGui::BeginPopup("FileMenuPopup")) {
        if (ImGui::MenuItem("New (Default Scene)", "Ctrl+N")) {
            init_default_scene();
            g_statusMsg = "Created new default scene";
        }
        if (ImGui::MenuItem("Splash Screen...")) {
            g_showSplashModal = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save Blender File (.blend)", "Ctrl+S")) {
            std::string path = "sdmc:/NexvynOS/Exports/scene.blend";
            if (BlenderIO::saveBlendFile(g_scene, path)) {
                g_lastExportPath = path;
                g_exportSuccessModal = true;
                g_statusMsg = "Saved: " + path;
            }
        }
        if (ImGui::MenuItem("Export Wavefront (.obj)...")) {
            std::string path = "sdmc:/NexvynOS/Exports/scene.obj";
            if (BlenderIO::exportOBJ(g_scene, path)) {
                g_lastExportPath = path;
                g_exportSuccessModal = true;
                g_statusMsg = "Exported OBJ: " + path;
            }
        }
        if (ImGui::MenuItem("Export glTF 2.0 (.gltf)...")) {
            std::string path = "sdmc:/NexvynOS/Exports/scene.gltf";
            if (BlenderIO::exportGLTF(g_scene, path)) {
                g_lastExportPath = path;
                g_exportSuccessModal = true;
                g_statusMsg = "Exported glTF: " + path;
            }
        }
        if (ImGui::MenuItem("Export STL (.stl)...")) {
            std::string path = "sdmc:/NexvynOS/Exports/scene.stl";
            if (BlenderIO::exportSTL(g_scene, path)) {
                g_lastExportPath = path;
                g_exportSuccessModal = true;
                g_statusMsg = "Exported STL: " + path;
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit Blender", "B / Esc")) {
            quit = true;
        }
        ImGui::EndPopup();
    }

    drawMenuBtn("Edit", "EditMenuPopup");
    ImGui::SameLine(0, NXN_S(2.0f));
    if (ImGui::BeginPopup("EditMenuPopup")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) { g_statusMsg = "Undo last action"; }
        if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z")) { g_statusMsg = "Redo action"; }
        ImGui::Separator();
        if (ImGui::MenuItem("Preferences...")) {}
        ImGui::EndPopup();
    }

    drawMenuBtn("Render", "RenderMenuPopup");
    ImGui::SameLine(0, NXN_S(2.0f));
    if (ImGui::BeginPopup("RenderMenuPopup")) {
        if (ImGui::MenuItem("Render Image", "F12")) {
            g_statusMsg = "Rendered active camera view (EEVEE Next / OpenGL 4.3)";
        }
        if (ImGui::MenuItem("Render Animation", "Ctrl+F12")) {
            g_statusMsg = "Started animation render";
        }
        ImGui::EndPopup();
    }

    drawMenuBtn("Window", "WindowMenuPopup");
    ImGui::SameLine(0, NXN_S(2.0f));
    if (ImGui::BeginPopup("WindowMenuPopup")) {
        if (ImGui::MenuItem("Toggle Fullscreen", "Alt+F11")) {
            static bool s_fs = true;
            s_fs = !s_fs;
            nxn_window_set_mode(s_fs ? NXN_WIN_FULLSCREEN : NXN_WIN_NORMAL);
        }
        if (ImGui::MenuItem("N3-N4 Telemetry", "F3")) {
            g_showTelemetryModal = true;
        }
        ImGui::EndPopup();
    }

    drawMenuBtn("Help", "HelpMenuPopup");
    ImGui::SameLine(0, NXN_S(2.0f));
    if (ImGui::BeginPopup("HelpMenuPopup")) {
        if (ImGui::MenuItem("Splash Screen")) {
            g_showSplashModal = true;
        }
        if (ImGui::MenuItem("About Blender 4.2 LTS NX")) {
            g_showAboutModal = true;
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // Blender 4.2 Workspace Tabs
    const char* workspaces[] = { "Layout", "Modeling", "Sculpting", "UV Edit", "Shading", "Anim", "GeoNodes" };
    for (int i = 0; i < 7; ++i) {
        bool active = (g_activeWorkspace == i);
        if (BlenderWidgets::drawWorkspaceTab(workspaces[i], active)) {
            switch_workspace(i);
        }
        ImGui::SameLine(0, NXN_S(2.0f));
    }

    // Right side: N3-N4 Performance Badge
    float avail = ImGui::GetContentRegionAvail().x;
    if (avail > NXN_S(115.0f)) {
        ImGui::SameLine(ImGui::GetWindowWidth() - NXN_S(115.0f));
        char perfStr[32];
        snprintf(perfStr, sizeof perfStr, "N4  %0.1f FPS", g_fps);
        ImGui::TextColored(ImVec4(0.40f, 0.85f, 0.50f, 1.0f), "%s", perfStr);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// ----------------------------------------------------------------------
// 3. 3D VIEWPORT HEADER BAR (Mode, Menus, Overlays, Shading Spheres)
// ----------------------------------------------------------------------
static void draw_viewport_header() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.188f, 0.188f, 0.188f, 1.0f));
    ImGui::BeginChild("ViewportHeader", ImVec2(0, NXN_S(30.0f)), false, ImGuiWindowFlags_NoScrollbar);

    // Mode dropdown button (Object / Edit / Sculpt)
    const char* modeName = "Object Mode v";
    ImVec4 modeCol = ImVec4(0.26f, 0.26f, 0.27f, 1.0f);
    if (g_scene.editMode == MODE_EDIT_VERTEX || g_scene.editMode == MODE_EDIT_EDGE || g_scene.editMode == MODE_EDIT_FACE) {
        modeName = "Edit Mode v";
        modeCol = ImVec4(0.85f, 0.45f, 0.10f, 1.0f);
    } else if (g_scene.editMode == MODE_SCULPT) {
        modeName = "Sculpt Mode v";
        modeCol = ImVec4(0.278f, 0.447f, 0.702f, 1.0f);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, modeCol);
    if (ImGui::Button(modeName, ImVec2(NXN_S(124.0f), NXN_S(24.0f)))) {
        ImGui::OpenPopup("ModeSelectorPopup");
    }
    ImGui::PopStyleColor();

    if (ImGui::BeginPopup("ModeSelectorPopup")) {
        if (ImGui::MenuItem("Object Mode", "Tab")) g_scene.editMode = MODE_OBJECT;
        if (ImGui::MenuItem("Edit Mode", "Tab")) g_scene.editMode = MODE_EDIT_VERTEX;
        if (ImGui::MenuItem("Sculpt Mode")) {
            g_scene.editMode = MODE_SCULPT;
            g_scene.solidLighting = LIGHT_MATCAP;
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine(0, NXN_S(4.0f));
    if (BlenderWidgets::drawMenuButton("View")) {
        ImGui::OpenPopup("ViewportViewPopup");
    }
    if (ImGui::BeginPopup("ViewportViewPopup")) {
        if (ImGui::MenuItem("Frame Selected", "Numpad .")) g_viewport.resetView();
        ImGui::Separator();
        if (ImGui::MenuItem("View Front", "Numpad 1")) g_viewport.setViewFront();
        if (ImGui::MenuItem("View Right", "Numpad 3")) g_viewport.setViewSide();
        if (ImGui::MenuItem("View Top", "Numpad 7")) g_viewport.setViewTop();
        ImGui::EndPopup();
    }

    ImGui::SameLine(0, NXN_S(2.0f));
    if (BlenderWidgets::drawMenuButton("Select")) {
        ImGui::OpenPopup("ViewportSelectPopup");
    }
    if (ImGui::BeginPopup("ViewportSelectPopup")) {
        if (ImGui::MenuItem("All", "A")) {
            for (auto& o : g_scene.objects) o.selected = true;
        }
        if (ImGui::MenuItem("None", "Alt+A")) {
            for (auto& o : g_scene.objects) o.selected = false;
        }
        if (ImGui::MenuItem("Invert", "Ctrl+I")) {
            for (auto& o : g_scene.objects) o.selected = !o.selected;
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine(0, NXN_S(2.0f));

    // Add Menu with full primitives
    if (BlenderWidgets::drawMenuButton("Add")) {
        ImGui::OpenPopup("AddViewportPopup");
    }
    if (ImGui::BeginPopup("AddViewportPopup")) {
        if (ImGui::BeginMenu("Mesh")) {
            if (ImGui::MenuItem("Plane")) add_plane();
            if (ImGui::MenuItem("Cube")) add_cube();
            if (ImGui::MenuItem("UV Sphere")) add_uv_sphere();
            if (ImGui::MenuItem("Ico Sphere")) add_ico_sphere();
            if (ImGui::MenuItem("Cylinder")) add_cylinder();
            if (ImGui::MenuItem("Cone")) add_cone();
            if (ImGui::MenuItem("Torus")) add_torus();
            ImGui::Separator();
            if (ImGui::MenuItem("Monkey (Suzanne)")) add_suzanne();
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Camera")) {
            SceneObject cam;
            cam.name = "Camera";
            cam.type = OBJ_CAMERA;
            cam.location = g_scene.cursor3D + Vec3(0, 2, 4);
            cam.mesh = Primitives::createCameraWireframe();
            g_scene.objects.push_back(cam);
            g_scene.selectObject((int)g_scene.objects.size() - 1);
        }
        if (ImGui::MenuItem("Point Light")) {
            SceneObject l;
            l.name = "Point Light";
            l.type = OBJ_LIGHT;
            l.location = g_scene.cursor3D + Vec3(2, 4, 2);
            l.mesh = Primitives::createLightWireframe();
            g_scene.objects.push_back(l);
            g_scene.selectObject((int)g_scene.objects.size() - 1);
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine(0, NXN_S(2.0f));
    if (BlenderWidgets::drawMenuButton("Object")) {
        ImGui::OpenPopup("ViewportObjectPopup");
    }
    if (ImGui::BeginPopup("ViewportObjectPopup")) {
        if (ImGui::MenuItem("Duplicate", "Shift+D")) {
            SceneObject* so = g_scene.getSelectedObject();
            if (so) {
                SceneObject dup = *so;
                dup.name = so->name + ".001";
                dup.location = dup.location + Vec3(1.0f, 0.0f, 0.0f);
                g_scene.objects.push_back(dup);
                g_scene.selectObject((int)g_scene.objects.size() - 1);
            }
        }
        if (ImGui::MenuItem("Delete", "X / Del")) delete_selected();
        ImGui::Separator();
        if (ImGui::MenuItem("Subdivide", "W / Sub")) {
            SceneObject* sel = g_scene.getSelectedObject();
            if (sel && sel->type == OBJ_MESH) MeshOps::subdivide(sel->mesh);
        }
        if (ImGui::MenuItem("Shade Smooth")) g_statusMsg = "Set Shade Smooth";
        if (ImGui::MenuItem("Shade Flat")) g_statusMsg = "Set Shade Flat";
        ImGui::EndPopup();
    }

    // Viewport Overlays Dropdown Button (Grid, Axes, 3D Cursor)
    ImGui::SameLine(0, NXN_S(8.0f));
    if (ImGui::Button("Overlays v", ImVec2(NXN_S(84.0f), NXN_S(24.0f)))) {
        ImGui::OpenPopup("OverlaysPopup");
    }
    if (ImGui::BeginPopup("OverlaysPopup")) {
        ImGui::TextColored(ImVec4(0.92f, 0.47f, 0.05f, 1.0f), "Viewport Overlays");
        ImGui::Separator();
        ImGui::Checkbox("Floor Grid", &g_scene.showFloorGrid);
        ImGui::Checkbox("Axes (X, Y)", &g_scene.showAxes);
        ImGui::Checkbox("3D Cursor", &g_scene.show3DCursor);
        ImGui::Checkbox("Wireframe", &g_scene.showWireOverlay);
        ImGui::Checkbox("Cavity Accents", &g_scene.showCavity);
        ImGui::EndPopup();
    }

    // Right-aligned Shading Modes & Dropdown (Studio, MatCap, Wire, Material)
    float availX = ImGui::GetContentRegionAvail().x;
    if (availX > NXN_S(140.0f)) {
        ImGui::SameLine(ImGui::GetWindowWidth() - NXN_S(150.0f));
        BlenderWidgets::drawShadingSpheres(g_scene);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ----------------------------------------------------------------------
// 4. LEFT TOOLBAR (T-Panel: Modeling & Sculpting Brushes)
// ----------------------------------------------------------------------
static void draw_left_toolbar(float h) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.165f, 0.165f, 0.170f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(NXN_S(4.0f), NXN_S(6.0f)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, NXN_S(4.0f)));
    ImGui::BeginChild("LeftToolbar", ImVec2(NXN_S(44.0f), h), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetScrollX(0.0f);

    float btnSz = NXN_S(34.0f);

    if (g_scene.editMode == MODE_SCULPT) {
        auto drawBrushBtn = [&](const char* id, BlenderWidgets::ToolIconType icon, SculptBrush b, const char* tip) {
            bool active = (g_scene.sculpt.activeBrush == b);
            if (BlenderWidgets::drawToolIconButton(id, icon, active, tip, btnSz)) {
                g_scene.sculpt.activeBrush = b;
                g_statusMsg = std::string("Sculpt Brush: ") + tip;
            }
        };

        drawBrushBtn("ScDraw",    BlenderWidgets::ICON_SCULPT_DRAW,    BRUSH_DRAW,    "Draw Brush");
        drawBrushBtn("ScClay",    BlenderWidgets::ICON_SCULPT_CLAY,    BRUSH_CLAY,    "Clay Strips");
        drawBrushBtn("ScSmooth",  BlenderWidgets::ICON_SCULPT_SMOOTH,  BRUSH_SMOOTH,  "Smooth");
        drawBrushBtn("ScFlatten", BlenderWidgets::ICON_SCULPT_FLATTEN, BRUSH_FLATTEN, "Flatten");
        drawBrushBtn("ScInflate", BlenderWidgets::ICON_SCULPT_INFLATE, BRUSH_INFLATE, "Inflate");
        drawBrushBtn("ScGrab",    BlenderWidgets::ICON_SCULPT_GRAB,    BRUSH_GRAB,    "Grab / Move");

        ImGui::Separator();
        if (BlenderWidgets::drawToolIconButton("ScSymX", BlenderWidgets::ICON_SYMMETRY_X, g_scene.sculpt.symmetryX, "Mirror X-Axis Symmetry", btnSz)) {
            g_scene.sculpt.symmetryX = !g_scene.sculpt.symmetryX;
            g_statusMsg = g_scene.sculpt.symmetryX ? "Symmetry X: ON" : "Symmetry X: OFF";
        }
    } else {
        if (BlenderWidgets::drawToolIconButton("TSelect", BlenderWidgets::ICON_SELECT, g_activeTool == 0, "Select Box (B)", btnSz)) {
            g_activeTool = 0; g_statusMsg = "Tool: Select Box (B)";
        }
        if (BlenderWidgets::drawToolIconButton("TCursor", BlenderWidgets::ICON_CURSOR, g_activeTool == 1, "3D Cursor", btnSz)) {
            g_activeTool = 1; g_statusMsg = "Tool: 3D Cursor";
        }

        ImGui::Separator();

        if (BlenderWidgets::drawToolIconButton("TMove", BlenderWidgets::ICON_MOVE, g_activeTool == 2, "Move / Grab (G)", btnSz)) {
            g_activeTool = 2; g_statusMsg = "Tool: Move / Grab (G)";
        }
        if (BlenderWidgets::drawToolIconButton("TRotate", BlenderWidgets::ICON_ROTATE, g_activeTool == 3, "Rotate (R)", btnSz)) {
            g_activeTool = 3; g_statusMsg = "Tool: Rotate (R)";
        }
        if (BlenderWidgets::drawToolIconButton("TScale", BlenderWidgets::ICON_SCALE, g_activeTool == 4, "Scale (S)", btnSz)) {
            g_activeTool = 4; g_statusMsg = "Tool: Scale (S)";
        }

        ImGui::Separator();

        if (BlenderWidgets::drawToolIconButton("TExt", BlenderWidgets::ICON_EXTRUDE, false, "Extrude Region (E)", btnSz)) {
            SceneObject* sel = g_scene.getSelectedObject();
            if (sel && sel->type == OBJ_MESH && !sel->mesh.faces.empty()) {
                MeshOps::extrudeFace(sel->mesh, 0, 0.5f);
                g_statusMsg = "Extruded Face on " + sel->name;
            }
        }
        if (BlenderWidgets::drawToolIconButton("TIns", BlenderWidgets::ICON_INSET, false, "Inset Faces (I)", btnSz)) {
            SceneObject* sel = g_scene.getSelectedObject();
            if (sel && sel->type == OBJ_MESH && !sel->mesh.faces.empty()) {
                MeshOps::insetFace(sel->mesh, 0, 0.25f);
                g_statusMsg = "Inset Face on " + sel->name;
            }
        }
        if (BlenderWidgets::drawToolIconButton("TSub", BlenderWidgets::ICON_SUBDIVIDE, false, "Subdivide Mesh", btnSz)) {
            SceneObject* sel = g_scene.getSelectedObject();
            if (sel && sel->type == OBJ_MESH) {
                MeshOps::subdivide(sel->mesh);
                g_statusMsg = "Subdivided: " + sel->name;
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

// ----------------------------------------------------------------------
// 5. RIGHT COLUMN - OUTLINER EDITOR (Top Half)
// ----------------------------------------------------------------------
static void draw_outliner_editor() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.165f, 0.165f, 0.170f, 1.0f));
    ImGui::BeginChild("OutlinerHeader", ImVec2(0, NXN_S(28.0f)), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.88f, 0.88f, 0.90f, 1.0f), "Outliner");
    ImGui::SameLine(NXN_S(72.0f));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - NXN_S(4.0f));
    ImGui::InputTextWithHint("##Search", "Search...", g_outlinerFilter, sizeof(g_outlinerFilter));
    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (ImGui::TreeNodeEx("Scene Collection", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::TreeNodeEx("Collection", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            for (size_t i = 0; i < g_scene.objects.size(); ++i) {
                auto& obj = g_scene.objects[i];

                if (g_outlinerFilter[0] != '\0') {
                    if (obj.name.find(g_outlinerFilter) == std::string::npos) continue;
                }

                ImGui::PushID((int)i);
                bool isSelected = ((int)i == g_scene.selectedIndex);

                ImVec2 rowPos = ImGui::GetCursorScreenPos();
                float rowW = ImGui::GetContentRegionAvail().x;
                float eyeSpace = NXN_S(24.0f);

                // Selectable row leaving room on the right for the Eye icon
                char selLabel[128];
                snprintf(selLabel, sizeof selLabel, "     %s", obj.name.c_str());
                if (ImGui::Selectable(selLabel, isSelected, ImGuiSelectableFlags_AllowOverlap, ImVec2(rowW - eyeSpace, NXN_S(18.0f)))) {
                    g_scene.selectObject((int)i);
                }

                // Draw Blender Object Vector Icon inside the row padding
                BlenderWidgets::drawOutlinerObjectIcon(dl, ImVec2(rowPos.x + NXN_S(4.0f), rowPos.y + NXN_S(2.0f)), obj.type, isSelected);

                // Eye visibility toggle button on the right
                ImGui::SameLine(rowW - NXN_S(18.0f));
                ImVec2 eyePos = ImGui::GetCursorScreenPos();
                if (ImGui::InvisibleButton("##eye", ImVec2(NXN_S(18.0f), NXN_S(18.0f)))) {
                    obj.visible = !obj.visible;
                }
                BlenderWidgets::drawEyeIcon(dl, ImVec2(eyePos.x + NXN_S(8.0f), eyePos.y + NXN_S(9.0f)), obj.visible);

                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }
}

// ----------------------------------------------------------------------
// 6. RIGHT COLUMN - PROPERTIES EDITOR (Bottom Half, Vertical Icon Rail)
// ----------------------------------------------------------------------
static void draw_properties_content() {
    SceneObject* sel = g_scene.getSelectedObject();

    // Tab 0: Tool Tab
    if (g_curPropTab == TAB_TOOL) {
        if (g_scene.editMode == MODE_SCULPT) {
            if (ImGui::CollapsingHeader("Active Sculpt Brush", ImGuiTreeNodeFlags_DefaultOpen)) {
                const char* brushNames[] = { "Draw", "Clay", "Smooth", "Flatten", "Inflate", "Grab" };
                ImGui::Text("Brush: %s", brushNames[(int)g_scene.sculpt.activeBrush]);
                BlenderWidgets::drawSliderProperty("Radius", g_scene.sculpt.brushRadius, 0.05f, 2.0f);
                BlenderWidgets::drawSliderProperty("Strength", g_scene.sculpt.brushStrength, 0.01f, 1.0f);
                ImGui::Checkbox("X-Axis Symmetry", &g_scene.sculpt.symmetryX);
            }
        } else {
            ImGui::TextDisabled("Active Tool Settings");
            ImGui::Separator();
            const char* toolNames[] = { "Select Box", "3D Cursor", "Move", "Rotate", "Scale" };
            ImGui::Text("Tool: %s", toolNames[g_activeTool]);
            ImGui::TextDisabled("Enter Sculpt Mode for brush options.");
        }
    }
    // Tab 1: Render Tab (EEVEE Next / OpenGL 4.3)
    else if (g_curPropTab == TAB_RENDER) {
        if (ImGui::CollapsingHeader("Render Engine", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextColored(ImVec4(0.92f, 0.50f, 0.10f, 1.0f), "EEVEE Next (OpenGL 4.3)");
            static int vpSamples = 64;
            static int rdrSamples = 128;
            BlenderWidgets::drawIntSliderProperty("Viewport", vpSamples, 16, 256);
            BlenderWidgets::drawIntSliderProperty("Render", rdrSamples, 32, 512);
        }
        if (ImGui::CollapsingHeader("Color Management", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("View: AgX");
            ImGui::Text("Look: High Contrast");
        }
    }
    // Tab 2: World Tab
    else if (g_curPropTab == TAB_WORLD) {
        if (ImGui::CollapsingHeader("Surface", ImGuiTreeNodeFlags_DefaultOpen)) {
            static Vec4 worldCol(0.05f, 0.05f, 0.06f, 1.0f);
            BlenderWidgets::drawColorProperty("Color", worldCol);
        }
    }
    // Tab 3: Object Tab
    else if (g_curPropTab == TAB_OBJECT) {
        if (!sel) {
            ImGui::TextDisabled("No active object selected");
            return;
        }
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            BlenderWidgets::drawTransformXYZ("Location", sel->location, 0.05f, "m");
            BlenderWidgets::drawTransformXYZ("Rotation", sel->rotation, 1.0f, "°");
            BlenderWidgets::drawTransformXYZ("Scale", sel->scale, 0.05f, "");
        }
        if (sel->type == OBJ_MESH) {
            if (ImGui::CollapsingHeader("Geometry Info", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextDisabled("Vertices: %d", (int)sel->mesh.vertices.size());
                ImGui::TextDisabled("Faces:    %d", (int)sel->mesh.faces.size());
                ImGui::TextDisabled("Edges:    %d", (int)sel->mesh.edges.size());
            }
        }
        if (ImGui::Button("Delete Object (X)", ImVec2(-1, NXN_S(24.0f)))) {
            delete_selected();
        }
    }
    // Tab 4: Modifiers Tab
    else if (g_curPropTab == TAB_MODIFIERS) {
        if (!sel || sel->type != OBJ_MESH) {
            ImGui::TextDisabled("Select a mesh object to add modifiers");
            return;
        }

        if (ImGui::Button("Add Modifier v", ImVec2(-1, NXN_S(26.0f)))) {
            ImGui::OpenPopup("AddModifierPopup");
        }
        if (ImGui::BeginPopup("AddModifierPopup")) {
            if (ImGui::MenuItem("Subdivision Surface")) {
                sel->modifiers.push_back({"Subdivision", MOD_SUBSURF, true, 1, false, false, false, 0.1f, 2, 0.05f});
            }
            if (ImGui::MenuItem("Mirror")) {
                sel->modifiers.push_back({"Mirror", MOD_MIRROR, true, 1, true, false, false, 0.1f, 2, 0.05f});
            }
            if (ImGui::MenuItem("Solidify")) {
                sel->modifiers.push_back({"Solidify", MOD_SOLIDIFY, true, 1, false, false, false, 0.1f, 2, 0.05f});
            }
            if (ImGui::MenuItem("Wireframe")) {
                g_scene.shadingMode = SHADE_WIREFRAME;
            }
            ImGui::EndPopup();
        }

        // Active Modifiers List
        for (int mi = 0; mi < (int)sel->modifiers.size(); ++mi) {
            auto& mod = sel->modifiers[mi];
            ImGui::PushID(mi);

            bool headerOpen = ImGui::CollapsingHeader(mod.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            if (headerOpen) {
                if (mod.type == MOD_SUBSURF) {
                    BlenderWidgets::drawIntSliderProperty("Levels", mod.subsurfLevels, 1, 3);
                    if (ImGui::Button("Apply", ImVec2(NXN_S(72.0f), NXN_S(22.0f)))) {
                        MeshOps::subdivide(sel->mesh);
                        sel->modifiers.erase(sel->modifiers.begin() + mi);
                        g_statusMsg = "Applied Modifier: Subdivision Surface";
                        ImGui::PopID();
                        break;
                    }
                } else if (mod.type == MOD_MIRROR) {
                    ImGui::Checkbox("X", &mod.mirrorX); ImGui::SameLine();
                    ImGui::Checkbox("Y", &mod.mirrorY); ImGui::SameLine();
                    ImGui::Checkbox("Z", &mod.mirrorZ);
                    if (ImGui::Button("Apply", ImVec2(NXN_S(80.0f), NXN_S(22.0f)))) {
                        MeshOps::applyMirror(sel->mesh, mod.mirrorX, mod.mirrorY, mod.mirrorZ);
                        sel->modifiers.erase(sel->modifiers.begin() + mi);
                        g_statusMsg = "Applied Modifier: Mirror";
                        ImGui::PopID();
                        break;
                    }
                } else if (mod.type == MOD_SOLIDIFY) {
                    BlenderWidgets::drawSliderProperty("Thickness", mod.solidifyThickness, 0.01f, 0.5f);
                    if (ImGui::Button("Apply", ImVec2(NXN_S(72.0f), NXN_S(22.0f)))) {
                        MeshOps::extrudeFace(sel->mesh, 0, mod.solidifyThickness);
                        sel->modifiers.erase(sel->modifiers.begin() + mi);
                        g_statusMsg = "Applied Modifier: Solidify";
                        ImGui::PopID();
                        break;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove", ImVec2(NXN_S(80.0f), NXN_S(22.0f)))) {
                    sel->modifiers.erase(sel->modifiers.begin() + mi);
                    ImGui::PopID();
                    break;
                }
            }
            ImGui::PopID();
        }
    }
    // Tab 5: Material Tab (Blender 4.0+ Principled BSDF v2)
    else if (g_curPropTab == TAB_MATERIAL) {
        if (!sel || sel->type != OBJ_MESH) {
            ImGui::TextDisabled("Select a mesh object to view material");
            return;
        }

        if (ImGui::CollapsingHeader("Principled BSDF (Blender 4.2 Standard)", ImGuiTreeNodeFlags_DefaultOpen)) {
            BlenderWidgets::drawColorProperty("Base Color", sel->material.baseColor);
            BlenderWidgets::drawSliderProperty("Metallic", sel->material.metallic, 0.0f, 1.0f);
            BlenderWidgets::drawSliderProperty("Roughness", sel->material.roughness, 0.0f, 1.0f);
            BlenderWidgets::drawSliderProperty("IOR", sel->material.ior, 1.0f, 2.5f);
            BlenderWidgets::drawSliderProperty("Alpha", sel->material.alpha, 0.0f, 1.0f);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.90f, 1.0f), "Subsurface");
            BlenderWidgets::drawSliderProperty("Weight", sel->material.subsurfaceWeight, 0.0f, 1.0f);
            BlenderWidgets::drawSliderProperty("Scale", sel->material.subsurfaceScale, 0.01f, 1.0f);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.90f, 1.0f), "Coat & Sheen");
            BlenderWidgets::drawSliderProperty("Coat Weight", sel->material.coatWeight, 0.0f, 1.0f);
            BlenderWidgets::drawSliderProperty("Coat Rough", sel->material.coatRoughness, 0.0f, 1.0f);
            BlenderWidgets::drawSliderProperty("Sheen", sel->material.sheenWeight, 0.0f, 1.0f);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.90f, 1.0f), "Emission");
            BlenderWidgets::drawColorProperty("Color", sel->material.emissionColor);
            BlenderWidgets::drawSliderProperty("Strength", sel->material.emissionStrength, 0.0f, 20.0f);
        }
    }
}

static void draw_properties_editor() {
    // Left vertical Blender icon rail (never clips!)
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.125f, 0.125f, 0.128f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(NXN_S(3.0f), NXN_S(4.0f)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, NXN_S(4.0f)));
    ImGui::BeginChild("PropNavRail", ImVec2(NXN_S(30.0f), 0), false, ImGuiWindowFlags_NoScrollbar);

    if (BlenderWidgets::drawPropVerticalTab("PTabTool", 0, g_curPropTab == TAB_TOOL,      "Active Tool & Workspace")) g_curPropTab = TAB_TOOL;
    if (BlenderWidgets::drawPropVerticalTab("PTabRend", 1, g_curPropTab == TAB_RENDER,    "Render Properties (EEVEE Next)")) g_curPropTab = TAB_RENDER;
    if (BlenderWidgets::drawPropVerticalTab("PTabWrld", 2, g_curPropTab == TAB_WORLD,     "World Properties")) g_curPropTab = TAB_WORLD;
    if (BlenderWidgets::drawPropVerticalTab("PTabObj",  3, g_curPropTab == TAB_OBJECT,    "Object Properties")) g_curPropTab = TAB_OBJECT;
    if (BlenderWidgets::drawPropVerticalTab("PTabMod",  4, g_curPropTab == TAB_MODIFIERS, "Modifier Properties")) g_curPropTab = TAB_MODIFIERS;
    if (BlenderWidgets::drawPropVerticalTab("PTabMat",  5, g_curPropTab == TAB_MATERIAL,  "Material Properties")) g_curPropTab = TAB_MATERIAL;

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    ImGui::SameLine(0, NXN_S(4.0f));

    // Right scrollable properties body
    ImGui::BeginChild("PropBody", ImVec2(0, 0), false);
    draw_properties_content();
    ImGui::EndChild();
}

// ----------------------------------------------------------------------
// 7. BOTTOM TIMELINE & STATUS BAR
// ----------------------------------------------------------------------
static void draw_bottom_timeline() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
    ImGui::BeginChild("TimelineEditor", ImVec2(0, NXN_S(42.0f)), true, ImGuiWindowFlags_NoScrollbar);

    // Transport buttons
    if (ImGui::Button("<<", ImVec2(NXN_S(28.0f), NXN_S(24.0f)))) g_scene.currentFrame = 1;
    ImGui::SameLine();
    if (ImGui::Button(g_scene.isPlaying ? "||" : ">", ImVec2(NXN_S(28.0f), NXN_S(24.0f)))) {
        g_scene.isPlaying = !g_scene.isPlaying;
    }
    ImGui::SameLine();
    if (ImGui::Button(">>", ImVec2(NXN_S(28.0f), NXN_S(24.0f)))) g_scene.currentFrame = g_scene.totalFrames;

    ImGui::SameLine();
    ImGui::SetNextItemWidth(NXN_S(80.0f));
    ImGui::DragInt("##Frame", &g_scene.currentFrame, 1, 1, g_scene.totalFrames, "Frame: %d");

    ImGui::SameLine();
    ImGui::TextDisabled("1 / 250");

    ImGui::SameLine();
    // Keyframe Insert Button (I)
    if (ImGui::Button("Key (I)", ImVec2(NXN_S(54.0f), NXN_S(24.0f)))) {
        SceneObject* sel = g_scene.getSelectedObject();
        if (sel) {
            sel->keyframes.push_back({g_scene.currentFrame, sel->location, sel->rotation, sel->scale});
            g_statusMsg = "Inserted Keyframe on frame " + std::to_string(g_scene.currentFrame);
        }
    }

    ImGui::SameLine();
    ImGui::Separator();

    // Timeline Scrub Ruler
    ImGui::SameLine();
    ImVec2 scrubStart = ImGui::GetCursorScreenPos();
    float scrubW = ImGui::GetContentRegionAvail().x - NXN_S(10.0f);
    if (scrubW > NXN_S(60.0f)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float barH = NXN_S(20.0f);
        dl->AddRectFilled(scrubStart, ImVec2(scrubStart.x + scrubW, scrubStart.y + barH), IM_COL32(35, 35, 38, 255), NXN_S(2.0f));

        for (int f = 0; f <= 250; f += 25) {
            float frac = (float)f / 250.0f;
            float tx = scrubStart.x + frac * scrubW;
            dl->AddLine(ImVec2(tx, scrubStart.y), ImVec2(tx, scrubStart.y + NXN_S(6.0f)), IM_COL32(80, 80, 85, 255));
        }

        float playheadFrac = (float)g_scene.currentFrame / (float)g_scene.totalFrames;
        float px = scrubStart.x + playheadFrac * scrubW;
        dl->AddLine(ImVec2(px, scrubStart.y), ImVec2(px, scrubStart.y + barH), IM_COL32(70, 140, 240, 255), 2.0f);
        dl->AddCircleFilled(ImVec2(px, scrubStart.y + NXN_S(3.0f)), NXN_S(4.0f), IM_COL32(70, 140, 240, 255));
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

static void draw_bottom_status_bar() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.114f, 0.114f, 0.114f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(NXN_S(6.0f), NXN_S(2.0f)));
    ImGui::BeginChild("StatusBar", ImVec2(0, NXN_S(24.0f)), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetScrollX(0.0f);

    BlenderWidgets::drawControlHint("L-Stick", "Orbit");
    BlenderWidgets::drawControlHint("R-Stick", "Pan");
    BlenderWidgets::drawControlHint("ZL/ZR", "Zoom");
    BlenderWidgets::drawControlHint("L/R", "Workspace");
    BlenderWidgets::drawControlHint("-", "Mode");
    BlenderWidgets::drawControlHint("+", "Play");
    BlenderWidgets::drawControlHint("X", "Delete");

    float avail = ImGui::GetContentRegionAvail().x;
    if (avail > NXN_S(225.0f)) {
        ImGui::SameLine(ImGui::GetWindowWidth() - NXN_S(225.0f));
        SceneObject* sel = g_scene.getSelectedObject();
        int vCount = sel ? (int)sel->mesh.vertices.size() : 0;
        int fCount = sel ? (int)sel->mesh.faces.size() : 0;
        ImGui::TextDisabled("Verts: %d | Faces: %d | v4.2", vCount, fCount);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// ----------------------------------------------------------------------
// 7B. WORKSPACE EDITORS: 3D VIEWPORT, GEOMETRY NODES, UV EDIT, DOPE SHEET
// ----------------------------------------------------------------------
static void draw_viewport_contents() {
    draw_viewport_header();

    ImVec2 curSize = ImGui::GetContentRegionAvail();
    int curW = (int)curSize.x;
    int curH = (int)curSize.y;

    if (curW > 10 && curH > 10) {
        g_viewport.resize(curW, curH);
        g_viewport.render(g_scene);

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImTextureID texID = (ImTextureID)(intptr_t)g_viewport.getTextureID();
        ImGui::Image(texID, curSize, ImVec2(0, 1), ImVec2(1, 0));

        // Blender Top-Left Watermark
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddText(ImVec2(canvasPos.x + NXN_S(12.0f), canvasPos.y + NXN_S(10.0f)), IM_COL32(230, 230, 230, 220), "User Perspective");
        SceneObject* curSel = g_scene.getSelectedObject();
        char metaStr[128];
        snprintf(metaStr, sizeof metaStr, "(1) Collection | %s", curSel ? curSel->name.c_str() : "None");
        dl->AddText(ImVec2(canvasPos.x + NXN_S(12.0f), canvasPos.y + NXN_S(26.0f)), IM_COL32(160, 160, 165, 200), metaStr);

        // Dynamic 3D Navigation Gizmo Overlay (Synced with Viewport Yaw/Pitch)
        float gizmoX = canvasPos.x + curSize.x - NXN_S(46.0f);
        float gizmoY = canvasPos.y + NXN_S(44.0f);
        float gRad = NXN_S(28.0f);
        dl->AddCircleFilled(ImVec2(gizmoX, gizmoY), gRad + NXN_S(4.0f), IM_COL32(28, 28, 30, 140), 28);

        float yawR = g_viewport.getYaw() * 3.14159265f / 180.0f;
        float pitchR = g_viewport.getPitch() * 3.14159265f / 180.0f;
        float cy = cosf(yawR), sy = sinf(yawR);
        float cp = cosf(pitchR), sp = sinf(pitchR);

        struct AxisProj { float sx, sy, sz; ImU32 col; const char* lbl; };
        auto projAxis = [&](float ax, float ay, float az, ImU32 col, const char* lbl) -> AxisProj {
            float rx = ax * cy - az * sy;
            float rz = ax * sy + az * cy;
            float ry = ay * cp - rz * sp;
            float depth = ay * sp + rz * cp;
            return { rx * gRad * 0.78f, -ry * gRad * 0.78f, depth, col, lbl };
        };
        AxisProj axes[3] = {
            projAxis(1, 0, 0, IM_COL32(230, 70, 75, 255), "X"),
            projAxis(0, 1, 0, IM_COL32(95, 205, 80, 255), "Z"), // Blender Z-up visual label
            projAxis(0, 0, 1, IM_COL32(70, 145, 240, 255), "Y")
        };
        std::sort(axes, axes + 3, [](const AxisProj& a, const AxisProj& b) { return a.sz < b.sz; });
        for (int ai = 0; ai < 3; ++ai) {
            ImVec2 tip(gizmoX + axes[ai].sx, gizmoY + axes[ai].sy);
            ImVec2 neg(gizmoX - axes[ai].sx * 0.65f, gizmoY - axes[ai].sy * 0.65f);
            dl->AddCircleFilled(neg, NXN_S(4.5f), IM_COL32(45, 45, 50, 180), 12);
            dl->AddLine(ImVec2(gizmoX, gizmoY), tip, axes[ai].col, NXN_S(2.0f));
            dl->AddCircleFilled(tip, NXN_S(7.5f), axes[ai].col, 16);
            ImVec2 tsz = ImGui::CalcTextSize(axes[ai].lbl);
            dl->AddText(ImVec2(tip.x - tsz.x * 0.5f, tip.y - tsz.y * 0.5f), IM_COL32(15, 15, 18, 255), axes[ai].lbl);
        }

        // Mouse & Touch Interaction
        if (ImGui::IsItemHovered()) {
            if (g_scene.editMode == MODE_SCULPT && ImGui::IsMouseDown(0)) {
                // Interactive Sculpting on mesh surface!
                ImVec2 mpos = ImGui::GetMousePos();
                ImVec2 vmin = ImGui::GetItemRectMin();
                float relX = mpos.x - vmin.x;
                float relY = mpos.y - vmin.y;

                Vec3 rayOrigin, rayDir;
                g_viewport.getRayFromScreen(relX, relY, rayOrigin, rayDir);

                SceneObject* selObj = g_scene.getSelectedObject();
                if (selObj && selObj->type == OBJ_MESH) {
                    Vec3 hitPos, hitNorm;
                    if (g_viewport.raycastSurface(*selObj, rayOrigin, rayDir, hitPos, hitNorm)) {
                        MeshOps::applySculptBrush(selObj->mesh, hitPos, hitNorm,
                                                  g_scene.sculpt.activeBrush,
                                                  g_scene.sculpt.brushRadius,
                                                  g_scene.sculpt.brushStrength,
                                                  g_scene.sculpt.symmetryX);
                    }
                }
            } else if (ImGui::IsMouseDragging(0)) {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                g_viewport.orbit(delta.x * 0.5f, delta.y * 0.5f);
            } else if (ImGui::IsMouseDragging(1) || ImGui::IsMouseDragging(2)) {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                g_viewport.pan(-delta.x * 0.01f, delta.y * 0.01f);
            }

            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) g_viewport.zoom(-wheel * 0.5f);

            if (g_scene.editMode != MODE_SCULPT && ImGui::IsMouseClicked(0)) {
                ImVec2 mpos = ImGui::GetMousePos();
                ImVec2 vmin = ImGui::GetItemRectMin();
                float relX = mpos.x - vmin.x;
                float relY = mpos.y - vmin.y;
                int pick = g_viewport.pickObject(g_scene, relX, relY);
                if (pick >= 0) {
                    g_scene.selectObject(pick);
                    g_statusMsg = "Selected: " + g_scene.objects[pick].name;
                }
            }
        }
    }
}

static void draw_geometry_nodes_editor(float w, float h) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.13f, 1.0f));
    ImGui::BeginChild("GeoNodesPane", ImVec2(w, h), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // 1. Header bar
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
    ImGui::BeginChild("GeoNodesHeader", ImVec2(0, NXN_S(26.0f)), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPosX(NXN_S(6.0f));
    ImGui::TextColored(ImVec4(0.92f, 0.47f, 0.05f, 1.0f), "[Geometry Node Tree]");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("Suzanne_GeoModifier");
    ImGui::SameLine();
    ImGui::TextDisabled("[2 Users]");
    ImGui::SameLine(ImGui::GetWindowWidth() - NXN_S(140.0f));
    ImGui::TextColored(ImVec4(0.40f, 0.85f, 0.50f, 1.0f), "* EEVEE Next Eval");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    // 2. Node Graph Canvas
    ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
    ImVec2 canvasSz = ImGui::GetContentRegionAvail();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Subtle grid background (dots)
    float gridStep = NXN_S(24.0f);
    for (float x = canvasP0.x + NXN_S(12.0f); x < canvasP0.x + canvasSz.x; x += gridStep) {
        for (float y = canvasP0.y + NXN_S(12.0f); y < canvasP0.y + canvasSz.y; y += gridStep) {
            dl->AddCircleFilled(ImVec2(x, y), NXN_S(1.0f), IM_COL32(50, 50, 55, 180));
        }
    }

    // Helper lambda to draw Blender Node
    auto drawBlenderNode = [&](ImVec2 pos, ImVec2 sz, const char* title, ImU32 headerCol) {
        ImVec2 pMax = ImVec2(pos.x + sz.x, pos.y + sz.y);
        // Node shadow & background
        dl->AddRectFilled(ImVec2(pos.x + 2, pos.y + 2), ImVec2(pMax.x + 2, pMax.y + 2), IM_COL32(10, 10, 12, 100), NXN_S(6.0f));
        dl->AddRectFilled(pos, pMax, IM_COL32(38, 38, 42, 255), NXN_S(6.0f));
        dl->AddRect(pos, pMax, IM_COL32(70, 70, 78, 255), NXN_S(6.0f), 0, 1.2f);
        // Header
        float hdrH = NXN_S(24.0f);
        dl->AddRectFilled(pos, ImVec2(pMax.x, pos.y + hdrH), headerCol, NXN_S(6.0f), ImDrawFlags_RoundCornersTop);
        dl->AddText(ImVec2(pos.x + NXN_S(10.0f), pos.y + NXN_S(5.0f)), IM_COL32(240, 240, 245, 255), title);
    };

    // Helper for sockets
    auto drawSocket = [&](ImVec2 center, ImU32 col, const char* label, bool isOutput) {
        dl->AddCircleFilled(center, NXN_S(4.5f), col);
        dl->AddCircle(center, NXN_S(4.5f), IM_COL32(20, 20, 22, 255), 12, 1.2f);
        if (isOutput) {
            ImVec2 txtSz = ImGui::CalcTextSize(label);
            dl->AddText(ImVec2(center.x - txtSz.x - NXN_S(8.0f), center.y - txtSz.y * 0.5f), IM_COL32(200, 200, 205, 255), label);
        } else {
            dl->AddText(ImVec2(center.x + NXN_S(8.0f), center.y - NXN_S(6.0f)), IM_COL32(200, 200, 205, 255), label);
        }
    };

    // Socket colors
    ImU32 colGeom = IM_COL32(100, 225, 120, 255); // Green (Geometry)
    ImU32 colFloat = IM_COL32(160, 160, 165, 255); // Gray (Float)
    ImU32 colMat = IM_COL32(230, 105, 170, 255);   // Pink (Material)

    // Node 1: Group Input
    ImVec2 n1Pos = ImVec2(canvasP0.x + NXN_S(16.0f), canvasP0.y + NXN_S(16.0f));
    ImVec2 n1Sz  = ImVec2(NXN_S(125.0f), NXN_S(85.0f));
    drawBlenderNode(n1Pos, n1Sz, "Group Input", IM_COL32(45, 95, 60, 255));
    ImVec2 n1OutGeom = ImVec2(n1Pos.x + n1Sz.x, n1Pos.y + NXN_S(38.0f));
    ImVec2 n1OutFac  = ImVec2(n1Pos.x + n1Sz.x, n1Pos.y + NXN_S(62.0f));
    drawSocket(n1OutGeom, colGeom, "Geometry", true);
    drawSocket(n1OutFac, colFloat, "Factor", true);

    // Node 2: Subdivide Mesh
    ImVec2 n2Pos = ImVec2(canvasP0.x + NXN_S(185.0f), canvasP0.y + NXN_S(18.0f));
    ImVec2 n2Sz  = ImVec2(NXN_S(145.0f), NXN_S(95.0f));
    drawBlenderNode(n2Pos, n2Sz, "Subdivide Mesh", IM_COL32(40, 75, 120, 255));
    ImVec2 n2InMesh  = ImVec2(n2Pos.x, n2Pos.y + NXN_S(38.0f));
    ImVec2 n2OutMesh = ImVec2(n2Pos.x + n2Sz.x, n2Pos.y + NXN_S(38.0f));
    drawSocket(n2InMesh, colGeom, "Mesh", false);
    drawSocket(n2OutMesh, colGeom, "Mesh", true);
    // Inner level pill
    dl->AddRectFilled(ImVec2(n2Pos.x + NXN_S(12.0f), n2Pos.y + NXN_S(58.0f)),
                      ImVec2(n2Pos.x + n2Sz.x - NXN_S(12.0f), n2Pos.y + NXN_S(80.0f)),
                      IM_COL32(26, 26, 30, 255), NXN_S(4.0f));
    dl->AddText(ImVec2(n2Pos.x + NXN_S(20.0f), n2Pos.y + NXN_S(62.0f)), IM_COL32(220, 220, 225, 255), "Level:  2");

    // Node 3: Set Material (wider + separated input/output rows so labels never overlap)
    ImVec2 n3Pos = ImVec2(canvasP0.x + NXN_S(365.0f), canvasP0.y + NXN_S(16.0f));
    ImVec2 n3Sz  = ImVec2(NXN_S(170.0f), NXN_S(118.0f));
    drawBlenderNode(n3Pos, n3Sz, "Set Material", IM_COL32(40, 75, 120, 255));
    ImVec2 n3OutGeom = ImVec2(n3Pos.x + n3Sz.x, n3Pos.y + NXN_S(38.0f));
    ImVec2 n3InGeom  = ImVec2(n3Pos.x, n3Pos.y + NXN_S(56.0f));
    ImVec2 n3InMat   = ImVec2(n3Pos.x, n3Pos.y + NXN_S(78.0f));
    drawSocket(n3OutGeom, colGeom, "Geometry", true);
    drawSocket(n3InGeom, colGeom, "Geometry", false);
    drawSocket(n3InMat, colMat, "Material", false);
    // Inner material name badge
    dl->AddRectFilled(ImVec2(n3Pos.x + NXN_S(24.0f), n3Pos.y + NXN_S(92.0f)),
                      ImVec2(n3Pos.x + n3Sz.x - NXN_S(14.0f), n3Pos.y + NXN_S(110.0f)),
                      IM_COL32(26, 26, 30, 255), NXN_S(3.0f));
    dl->AddText(ImVec2(n3Pos.x + NXN_S(30.0f), n3Pos.y + NXN_S(94.0f)), IM_COL32(235, 180, 100, 255), "SuzanneMat");

    // Node 4: Group Output
    ImVec2 n4Pos = ImVec2(canvasP0.x + NXN_S(580.0f), canvasP0.y + NXN_S(16.0f));
    ImVec2 n4Sz  = ImVec2(NXN_S(128.0f), NXN_S(80.0f));
    drawBlenderNode(n4Pos, n4Sz, "Group Output", IM_COL32(115, 45, 40, 255));
    ImVec2 n4InGeom = ImVec2(n4Pos.x, n4Pos.y + NXN_S(38.0f));
    drawSocket(n4InGeom, colGeom, "Geometry", false);

    // Blender Signature Bezier Noodle Connections
    auto drawNoodle = [&](ImVec2 p1, ImVec2 p2, ImU32 col) {
        float dx = std::max(std::abs(p2.x - p1.x) * 0.5f, NXN_S(35.0f));
        ImVec2 cp1 = ImVec2(p1.x + dx, p1.y);
        ImVec2 cp2 = ImVec2(p2.x - dx, p2.y);
        dl->AddBezierCubic(p1, cp1, cp2, p2, col, NXN_S(2.4f));
    };

    drawNoodle(n1OutGeom, n2InMesh, IM_COL32(100, 225, 120, 240));
    drawNoodle(n2OutMesh, n3InGeom, IM_COL32(100, 225, 120, 240));
    drawNoodle(n3OutGeom, n4InGeom, IM_COL32(100, 225, 120, 240));

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

static void draw_uv_editor(float w, float h) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.13f, 1.0f));
    ImGui::BeginChild("UVEditorPane", ImVec2(w, h), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Header
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
    ImGui::BeginChild("UVHeader", ImVec2(0, NXN_S(26.0f)), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPosX(NXN_S(6.0f));
    ImGui::TextColored(ImVec4(0.92f, 0.47f, 0.05f, 1.0f), "UV Editor");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("Suzanne_UVMap");
    ImGui::SameLine();
    ImGui::TextDisabled("1024x1024 (RGBA)");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 sz = ImGui::GetContentRegionAvail();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Checkerboard UV grid
    float sq = NXN_S(20.0f);
    int cols = (int)(sz.x / sq) + 1;
    int rows = (int)(sz.y / sq) + 1;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            ImU32 col = ((r + c) % 2 == 0) ? IM_COL32(24, 24, 27, 255) : IM_COL32(32, 32, 36, 255);
            dl->AddRectFilled(ImVec2(p0.x + c * sq, p0.y + r * sq),
                              ImVec2(p0.x + (c + 1) * sq, p0.y + (r + 1) * sq), col);
        }
    }

    // 0..1 UV Bounds box
    float boxMargin = NXN_S(20.0f);
    float boxW = std::min(sz.x - boxMargin * 2.0f, sz.y - boxMargin * 2.0f);
    if (boxW > 20.0f) {
        ImVec2 bMin = ImVec2(p0.x + (sz.x - boxW) * 0.5f, p0.y + (sz.y - boxW) * 0.5f);
        ImVec2 bMax = ImVec2(bMin.x + boxW, bMin.y + boxW);
        dl->AddRect(bMin, bMax, IM_COL32(220, 120, 30, 200), 0, 0, 1.5f);

        // Suzanne UV Islands representation (Unwrapped Wireframe layout)
        auto uvPt = [&](float u, float v) -> ImVec2 {
            return ImVec2(bMin.x + u * boxW, bMin.y + (1.0f - v) * boxW);
        };

        // Head island
        ImVec2 h1 = uvPt(0.35f, 0.40f);
        ImVec2 h2 = uvPt(0.65f, 0.40f);
        ImVec2 h3 = uvPt(0.70f, 0.75f);
        ImVec2 h4 = uvPt(0.50f, 0.88f);
        ImVec2 h5 = uvPt(0.30f, 0.75f);
        dl->AddQuad(h1, h2, h3, h5, IM_COL32(235, 160, 50, 220), 1.2f);
        dl->AddTriangle(h5, h3, h4, IM_COL32(235, 160, 50, 220), 1.2f);

        // Left Ear Island
        ImVec2 eL1 = uvPt(0.12f, 0.60f);
        ImVec2 eL2 = uvPt(0.24f, 0.55f);
        ImVec2 eL3 = uvPt(0.22f, 0.78f);
        dl->AddTriangle(eL1, eL2, eL3, IM_COL32(235, 160, 50, 220), 1.2f);

        // Right Ear Island
        ImVec2 eR1 = uvPt(0.76f, 0.55f);
        ImVec2 eR2 = uvPt(0.88f, 0.60f);
        ImVec2 eR3 = uvPt(0.78f, 0.78f);
        dl->AddTriangle(eR1, eR2, eR3, IM_COL32(235, 160, 50, 220), 1.2f);

        // Muzzle / Eyes
        ImVec2 m1 = uvPt(0.42f, 0.22f);
        ImVec2 m2 = uvPt(0.58f, 0.22f);
        ImVec2 m3 = uvPt(0.62f, 0.38f);
        ImVec2 m4 = uvPt(0.38f, 0.38f);
        dl->AddQuad(m1, m2, m3, m4, IM_COL32(235, 160, 50, 220), 1.2f);

        // UV Vertices dots
        const ImVec2 pts[] = { h1, h2, h3, h4, h5, eL1, eL2, eL3, eR1, eR2, eR3, m1, m2, m3, m4 };
        for (const auto& p : pts) {
            dl->AddCircleFilled(p, NXN_S(2.2f), IM_COL32(255, 200, 60, 255));
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

static void draw_dopesheet_editor(float w, float h) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.13f, 0.14f, 1.0f));
    ImGui::BeginChild("DopeSheetPane", ImVec2(w, h), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Header
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
    ImGui::BeginChild("DopeHeader", ImVec2(0, NXN_S(26.0f)), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPosX(NXN_S(6.0f));
    ImGui::TextColored(ImVec4(0.92f, 0.47f, 0.05f, 1.0f), "Dope Sheet");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("Action: SuzanneAction");
    ImGui::SameLine();
    ImGui::TextDisabled("[250 Frames @ 24fps]");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 sz = ImGui::GetContentRegionAvail();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    float channelW = NXN_S(140.0f);
    float timelineW = sz.x - channelW;
    float rowH = NXN_S(22.0f);

    const char* channels[] = {
        "v Summary",
        "  v Suzanne",
        "    v Transform",
        "      X Location",
        "      Y Rotation",
        "      Z Rotation"
    };
    ImU32 chanCols[] = {
        IM_COL32(235, 120, 30, 255),
        IM_COL32(240, 240, 245, 255),
        IM_COL32(180, 180, 185, 255),
        IM_COL32(220, 70, 70, 255),
        IM_COL32(70, 200, 70, 255),
        IM_COL32(70, 120, 230, 255)
    };

    // Draw channels on the left and timeline track on right
    for (int r = 0; r < 6; ++r) {
        float y = p0.y + r * rowH;
        ImU32 bgCol = (r % 2 == 0) ? IM_COL32(26, 26, 29, 255) : IM_COL32(22, 22, 25, 255);
        // Channel background
        dl->AddRectFilled(ImVec2(p0.x, y), ImVec2(p0.x + channelW, y + rowH), bgCol);
        dl->AddText(ImVec2(p0.x + NXN_S(6.0f), y + NXN_S(3.0f)), chanCols[r], channels[r]);
        // Track background
        dl->AddRectFilled(ImVec2(p0.x + channelW, y), ImVec2(p0.x + sz.x, y + rowH), bgCol);
        dl->AddLine(ImVec2(p0.x + channelW, y + rowH), ImVec2(p0.x + sz.x, y + rowH), IM_COL32(40, 40, 45, 255));
    }

    // Grid vertical lines for frames
    for (int f = 0; f <= 250; f += 25) {
        float frac = (float)f / 250.0f;
        float x = p0.x + channelW + frac * (timelineW - NXN_S(10.0f));
        dl->AddLine(ImVec2(x, p0.y), ImVec2(x, p0.y + 6 * rowH), IM_COL32(45, 45, 50, 200));
        char fstr[8];
        snprintf(fstr, sizeof fstr, "%d", f);
        dl->AddText(ImVec2(x + 2, p0.y + 1), IM_COL32(100, 100, 110, 180), fstr);
    }

    // Yellow Diamond Keyframes
    auto drawKeyDiamond = [&](int frame, int row) {
        float frac = (float)frame / 250.0f;
        float cx = p0.x + channelW + frac * (timelineW - NXN_S(10.0f));
        float cy = p0.y + row * rowH + rowH * 0.5f;
        float d = NXN_S(4.5f);
        dl->AddQuadFilled(ImVec2(cx, cy - d), ImVec2(cx + d, cy), ImVec2(cx, cy + d), ImVec2(cx - d, cy), IM_COL32(245, 200, 40, 255));
        dl->AddQuad(ImVec2(cx, cy - d), ImVec2(cx + d, cy), ImVec2(cx, cy + d), ImVec2(cx - d, cy), IM_COL32(30, 30, 35, 255), 1.0f);
    };

    // Pre-populated keyframes on frames 1, 40, 80, 120, 160, 200, 240
    int kFrames[] = { 1, 40, 80, 120, 160, 200, 240 };
    for (int kf : kFrames) {
        for (int r = 0; r < 6; ++r) {
            drawKeyDiamond(kf, r);
        }
    }

    // Playhead line
    float playFrac = (float)g_scene.currentFrame / (float)g_scene.totalFrames;
    float playheadX = p0.x + channelW + playFrac * (timelineW - NXN_S(10.0f));
    dl->AddLine(ImVec2(playheadX, p0.y), ImVec2(playheadX, p0.y + 6 * rowH), IM_COL32(70, 140, 240, 255), 2.0f);
    dl->AddCircleFilled(ImVec2(playheadX, p0.y + NXN_S(4.0f)), NXN_S(4.0f), IM_COL32(70, 140, 240, 255));

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ----------------------------------------------------------------------
// 8. MAIN UI DRAW DISPATCHER
// ----------------------------------------------------------------------
static bool draw_ui() {
    bool quit = false;
    BlenderTheme::applyTheme();

    ImGui::SetScrollX(0.0f);
    ImGui::SetScrollY(0.0f);

    draw_top_application_bar(quit);

    // Hardware Joy-Con & Nexvyn OS Full Controller Integration
    if (std::abs(g_in.lx) > 0.15f || std::abs(g_in.ly) > 0.15f) {
        g_viewport.orbit(g_in.lx * 2.5f, -g_in.ly * 2.5f);
    }
    if (std::abs(g_in.rx) > 0.15f || std::abs(g_in.ry) > 0.15f) {
        g_viewport.pan(g_in.rx * 0.05f, g_in.ry * 0.05f);
    }
    if (g_in.buttons & NXN_BTN_ZL) g_viewport.zoom(-0.1f);
    if (g_in.buttons & NXN_BTN_ZR) g_viewport.zoom(0.1f);

    if (g_in.pressed & NXN_BTN_UP)    g_viewport.setViewTop();
    if (g_in.pressed & NXN_BTN_DOWN)  g_viewport.resetView();
    if (g_in.pressed & NXN_BTN_RIGHT) g_viewport.setViewFront();
    if (g_in.pressed & NXN_BTN_LEFT)  g_viewport.setViewSide();

    if (g_in.pressed & NXN_BTN_L)      switch_workspace(g_activeWorkspace - 1);
    if (g_in.pressed & NXN_BTN_R)      switch_workspace(g_activeWorkspace + 1);
    if (g_in.pressed & NXN_BTN_PLUS)   g_scene.isPlaying = !g_scene.isPlaying;
    if (g_in.pressed & NXN_BTN_LSTICK) {
        g_scene.shadingMode = (ShadingMode)(((int)g_scene.shadingMode + 1) % 4);
    }
    if (g_in.pressed & NXN_BTN_X)     delete_selected();
    if (g_in.pressed & NXN_BTN_MINUS) toggle_mode();
    if (g_in.pressed & NXN_BTN_Y)     g_viewport.resetView();

    // USB Keyboard Blender Shortcuts
    if (!ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_Tab))   toggle_mode();
        if (ImGui::IsKeyPressed(ImGuiKey_Space)) g_scene.isPlaying = !g_scene.isPlaying;
        if (ImGui::IsKeyPressed(ImGuiKey_G))     { g_activeTool = 2; g_statusMsg = "Tool: Move (G)"; }
        if (ImGui::IsKeyPressed(ImGuiKey_R))     { g_activeTool = 3; g_statusMsg = "Tool: Rotate (R)"; }
        if (ImGui::IsKeyPressed(ImGuiKey_S))     { g_activeTool = 4; g_statusMsg = "Tool: Scale (S)"; }
    }

    // Dimensions
    float totalAvailW = ImGui::GetContentRegionAvail().x;
    float totalAvailH = ImGui::GetContentRegionAvail().y;
    float rightSidebarW = NXN_S(260.0f);
    float leftToolbarW = NXN_S(42.0f);
    float timelineH = NXN_S(42.0f);
    float statusH = NXN_S(22.0f);
    float mainAreaH = totalAvailH - timelineH - statusH - NXN_S(6.0f);

    float viewportW = totalAvailW - leftToolbarW - rightSidebarW - NXN_S(14.0f);
    if (viewportW < NXN_S(200.0f)) viewportW = NXN_S(200.0f);

    // 1. LEFT: TOOLBAR
    ImGui::SetCursorPosX(NXN_S(4.0f));
    draw_left_toolbar(mainAreaH);

    ImGui::SameLine(0, NXN_S(4.0f));

    // 2. CENTER: 3D VIEWPORT / EDITORS AREA
    if (g_activeWorkspace == 6) {
        // GeoNodes Workspace: Top 44% Viewport, Bottom 56% Geometry Nodes Editor
        float vpH = (mainAreaH - NXN_S(4.0f)) * 0.44f;
        float gnH = mainAreaH - vpH - NXN_S(4.0f);

        ImGui::BeginGroup();
        ImGui::BeginChild("ViewportEditorArea", ImVec2(viewportW, vpH), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetScrollX(0.0f);
        draw_viewport_contents();
        ImGui::EndChild();

        draw_geometry_nodes_editor(viewportW, gnH);
        ImGui::EndGroup();
    }
    else if (g_activeWorkspace == 3) {
        // UV Edit Workspace: Left 45% UV Editor, Right 55% 3D Viewport
        float uvW = (viewportW - NXN_S(4.0f)) * 0.45f;
        float vpW = viewportW - uvW - NXN_S(4.0f);

        ImGui::BeginGroup();
        draw_uv_editor(uvW, mainAreaH);

        ImGui::SameLine(0, NXN_S(4.0f));

        ImGui::BeginChild("ViewportEditorArea", ImVec2(vpW, mainAreaH), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetScrollX(0.0f);
        draw_viewport_contents();
        ImGui::EndChild();
        ImGui::EndGroup();
    }
    else if (g_activeWorkspace == 5) {
        // Anim Workspace: Top 55% Viewport, Bottom 45% Dope Sheet
        float vpH = (mainAreaH - NXN_S(4.0f)) * 0.55f;
        float dsH = mainAreaH - vpH - NXN_S(4.0f);

        ImGui::BeginGroup();
        ImGui::BeginChild("ViewportEditorArea", ImVec2(viewportW, vpH), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetScrollX(0.0f);
        draw_viewport_contents();
        ImGui::EndChild();

        draw_dopesheet_editor(viewportW, dsH);
        ImGui::EndGroup();
    }
    else {
        // Standard full-height 3D Viewport (Layout, Modeling, Sculpting, Shading)
        ImGui::BeginChild("ViewportEditorArea", ImVec2(viewportW, mainAreaH), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetScrollX(0.0f);
        draw_viewport_contents();
        ImGui::EndChild();
    }

    ImGui::SameLine();

    // 3. RIGHT COLUMN: OUTLINER + PROPERTIES
    ImGui::BeginChild("RightSidebarArea", ImVec2(rightSidebarW, mainAreaH), false, ImGuiWindowFlags_NoScrollbar);
    {
        float outlinerH = mainAreaH * 0.42f;
        float propertiesH = mainAreaH - outlinerH - NXN_S(6.0f);

        ImGui::BeginChild("OutlinerEditorPane", ImVec2(0, outlinerH), true);
        draw_outliner_editor();
        ImGui::EndChild();

        ImGui::BeginChild("PropertiesEditorPane", ImVec2(0, propertiesH), true);
        draw_properties_editor();
        ImGui::EndChild();
    }
    ImGui::EndChild();

    // 4. BOTTOM TIMELINE & STATUS BAR
    draw_bottom_timeline();
    draw_bottom_status_bar();

    // Modals
    draw_splash_screen();

    if (g_showAboutModal) {
        ImGui::OpenPopup("About Blender 4.2 LTS NX");
        g_showAboutModal = false;
    }
    if (ImGui::BeginPopupModal("About Blender 4.2 LTS NX", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.92f, 0.47f, 0.05f, 1.0f), "Blender-NX (4.2 LTS)");
        ImGui::Text("Port by sonerx7 | Nexvyn OS N3-N4");
        ImGui::Separator();
        ImGui::Text("Version: 4.2.0 LTS (N3-N4 Native Execution)");
        ImGui::Text("Renderer: Nexvyn Core OpenGL 4.3 / EEVEE Next");
        ImGui::Text("UI Engine: NXUI Semantic Tokens & Dear ImGui Integration");
        ImGui::Text("Memory Target: N4 Heavy Application Profile (256 MB)");
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(NXN_S(120.0f), 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (g_showTelemetryModal) {
        ImGui::OpenPopup("N3-N4 Telemetry");
        g_showTelemetryModal = false;
    }
    if (ImGui::BeginPopupModal("N3-N4 Telemetry", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.45f, 1.0f), "Nexvyn OS N3-N4 Execution Telemetry");
        ImGui::Separator();
        ImGui::Text("Frame Rate: %0.1f FPS", g_fps);
        ImGui::Text("Frame Latency: %0.2f ms", g_frameTimeMs);
        ImGui::Text("Total Frames: %llu", (unsigned long long)g_totalFramesRendered);
        ImGui::Text("Active Objects: %d", (int)g_scene.objects.size());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(NXN_S(100.0f), 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (g_exportSuccessModal) {
        ImGui::OpenPopup("Export Successful");
        g_exportSuccessModal = false;
    }
    if (ImGui::BeginPopupModal("Export Successful", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "File Exported Successfully!");
        ImGui::Text("Path: %s", g_lastExportPath.c_str());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(NXN_S(120.0f), 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    return quit;
}

static int app_main(int, char**) {
    if (nxn_display_open(NULL, &g_disp) != NXN_OK) {
        nxn_request_return(NXN_RETURN_ERROR, 1, "OpenGL display failed");
        return 1;
    }

    gladLoadGLLoader((GLADloadproc)nxn_display_proc);
    apply_tokens();
    nxn_display_size(g_disp, &g_w, &g_h);

    nxn_caps caps = NXN_STRUCT_INIT(nxn_caps);
    if (nxn_caps_get(&caps) == NXN_OK && (caps.features & NXN_CAP_WINDOW_CTL)) {
        nxn_window_set_title("Blender 4.2 LTS NX", "Suzanne.blend - N4 Core");
        nxn_window_set_mode(NXN_WIN_FULLSCREEN);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    nxn_imgui_enable_gamepad(io);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    BlenderTheme::applyTheme();
    g_viewport.init();
    init_default_scene();

    uint64_t last = nxn_time_ns();
    bool quit = false;

    while (!quit && !nxn_should_quit()) {
        nxn_event ev = NXN_STRUCT_INIT(nxn_event);
        while (nxn_event_poll(&ev) > 0) {
            nxn_imgui_input_event(ev);
            if (ev.type == NXN_EVENT_RESIZE) {
                if (nxn_display_resize(g_disp, ev.width, ev.height) == NXN_OK) {
                    nxn_display_size(g_disp, &g_w, &g_h);
                }
            } else if (ev.type == NXN_EVENT_THEME) {
                apply_tokens();
            } else if (ev.type == NXN_EVENT_QUIT) {
                quit = true;
            }
            ev = NXN_STRUCT_INIT(nxn_event);
        }

        nxn_input_poll(&g_in);
        if (g_in.actions & NXN_ACT_CANCEL) quit = true;

        uint64_t now = nxn_time_ns();
        float dt = (float)((now - last) / 1e9);
        if (dt > 0.0001f) {
            g_frameTimeMs = dt * 1000.0f;
            g_fps = 0.9f * g_fps + 0.1f * (1.0f / dt);
        }
        g_totalFramesRendered++;
        feed_imgui(dt);
        last = now;

        if (g_scene.isPlaying) {
            static float frameTimer = 0.0f;
            frameTimer += dt;
            float frameDuration = 1.0f / (float)(g_scene.fps > 0 ? g_scene.fps : 24);
            if (frameTimer >= frameDuration) {
                frameTimer = 0.0f;
                g_scene.currentFrame = (g_scene.currentFrame % g_scene.totalFrames) + 1;
                SceneObject* sel = g_scene.getSelectedObject();
                if (sel) {
                    sel->rotation.y += 1.0f;
                    if (sel->rotation.y > 360.0f) sel->rotation.y -= 360.0f;
                }
            }
        }

        nxn_os_frame_begin_ex(&g_tok, (int)g_w, (int)g_h, NXN_OS_FRAME_NO_WINDOW);
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2((float)g_w, (float)g_h));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("BlenderRoot", NULL,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar(2);

        if (draw_ui()) quit = true;

        ImGui::End();
        nxn_os_frame_end();

        glViewport(0, 0, (GLsizei)g_w, (GLsizei)g_h);
        glDisable(GL_SCISSOR_TEST);
        ImVec4 bg = nxn_os_clear_color(g_tok);
        glClearColor(bg.x, bg.y, bg.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        nxn_display_swap(g_disp);
    }

    g_viewport.shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    nxn_display_close(g_disp);
    return 0;
}

int main(int argc, char** argv) {
    return nxn_run(argc, argv, app_main, NULL);
}
