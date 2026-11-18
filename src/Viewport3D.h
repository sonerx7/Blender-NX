#pragma once
#include <glad/glad.h>
#include "Scene.h"

class Viewport3D {
public:
    Viewport3D();
    ~Viewport3D();

    bool init();
    void shutdown();
    void resize(int w, int h);
    void render(Scene& scene);

    GLuint getTextureID() const { return m_colorTex; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    // Camera Navigation
    void orbit(float deltaYaw, float deltaPitch);
    void pan(float deltaX, float deltaY);
    void zoom(float deltaDist);
    void resetView();
    void setViewFront();
    void setViewSide();
    void setViewTop();

    // Raycast pick & surface hit
    void getRayFromScreen(float mouseX, float mouseY, Vec3& outOrigin, Vec3& outDir);
    int pickObject(const Scene& scene, float mouseX, float mouseY);
    bool raycastSurface(const SceneObject& obj, const Vec3& rayOrigin, const Vec3& rayDir, Vec3& outHitPos, Vec3& outHitNormal);

    Vec3 getCameraEye() const;
    Vec3 getCameraTarget() const { return m_target; }
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }

private:
    void initShaders();
    void renderGrid(const Mat4& vp, bool showFloor, bool showAxes);
    void renderObject(const SceneObject& obj, const Mat4& vp, const Mat4& view, bool isSelected, const Scene& scene);
    void renderGizmo(const Vec3& pos, const Mat4& vp);
    void renderCursor3D(const Vec3& pos, const Mat4& vp);

    GLuint m_fbo = 0;
    GLuint m_colorTex = 0;
    GLuint m_depthRbo = 0;
    int m_width = 800;
    int m_height = 600;

    // Camera parameters
    Vec3 m_target = {0.0f, 0.0f, 0.0f};
    float m_yaw = 45.0f;     // degrees
    float m_pitch = 30.0f;   // degrees
    float m_distance = 6.0f; // distance to target

    // Shader programs
    GLuint m_progShaded = 0;
    GLuint m_progFlat = 0;

    // Uniform locations for Shaded program
    GLint m_uMVP = -1;
    GLint m_uModel = -1;
    GLint m_uView = -1;
    GLint m_uColor = -1;
    GLint m_uLightDir = -1;
    GLint m_uRoughness = -1;
    GLint m_uMetallic = -1;
    GLint m_uSpecular = -1;
    GLint m_uShadingMode = -1;
    GLint m_uLightingType = -1;
    GLint m_uMatCapType = -1;
    GLint m_uCavity = -1;
    GLint m_uXRay = -1;

    // Flat shader uniforms
    GLint m_uFlatMVP = -1;
    GLint m_uFlatColor = -1;
};
