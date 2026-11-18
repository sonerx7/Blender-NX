#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>

struct Vec2 {
    float x = 0.0f, y = 0.0f;
    Vec2() = default;
    Vec2(float _x, float _y) : x(_x), y(_y) {}
};

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3() = default;
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }

    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const {
        float l = length();
        return l > 1e-6f ? (*this / l) : Vec3(0, 1, 0);
    }

    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return Vec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    static float dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
};

struct Vec4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
    Vec4() = default;
    Vec4(float _x, float _y, float _z, float _w = 1.0f) : x(_x), y(_y), z(_z), w(_w) {}
};

struct Mat4 {
    float m[16];

    Mat4() {
        for (int i = 0; i < 16; ++i) m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }

    static Mat4 identity() { return Mat4(); }

    static Mat4 perspective(float fovRad, float aspect, float nearZ, float farZ) {
        Mat4 res;
        float tanHalfFov = std::tan(fovRad * 0.5f);
        for (int i = 0; i < 16; ++i) res.m[i] = 0.0f;
        res.m[0] = 1.0f / (aspect * tanHalfFov);
        res.m[5] = 1.0f / tanHalfFov;
        res.m[10] = -(farZ + nearZ) / (farZ - nearZ);
        res.m[11] = -1.0f;
        res.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
        return res;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 s = Vec3::cross(f, up).normalized();
        Vec3 u = Vec3::cross(s, f);

        Mat4 res;
        res.m[0] = s.x;  res.m[4] = s.y;  res.m[8]  = s.z;  res.m[12] = -Vec3::dot(s, eye);
        res.m[1] = u.x;  res.m[5] = u.y;  res.m[9]  = u.z;  res.m[13] = -Vec3::dot(u, eye);
        res.m[2] = -f.x; res.m[6] = -f.y; res.m[10] = -f.z; res.m[14] = Vec3::dot(f, eye);
        res.m[3] = 0.0f; res.m[7] = 0.0f; res.m[11] = 0.0f; res.m[15] = 1.0f;
        return res;
    }

    static Mat4 translation(const Vec3& v) {
        Mat4 res;
        res.m[12] = v.x;
        res.m[13] = v.y;
        res.m[14] = v.z;
        return res;
    }

    static Mat4 scaling(const Vec3& v) {
        Mat4 res;
        res.m[0] = v.x;
        res.m[5] = v.y;
        res.m[10] = v.z;
        return res;
    }

    static Mat4 rotationX(float rad) {
        Mat4 res;
        float c = std::cos(rad), s = std::sin(rad);
        res.m[5] = c;  res.m[9]  = -s;
        res.m[6] = s;  res.m[10] = c;
        return res;
    }

    static Mat4 rotationY(float rad) {
        Mat4 res;
        float c = std::cos(rad), s = std::sin(rad);
        res.m[0] = c;  res.m[8]  = s;
        res.m[2] = -s; res.m[10] = c;
        return res;
    }

    static Mat4 rotationZ(float rad) {
        Mat4 res;
        float c = std::cos(rad), s = std::sin(rad);
        res.m[0] = c;  res.m[4]  = -s;
        res.m[1] = s;  res.m[5]  = c;
        return res;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 out;
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += m[row + k * 4] * b.m[k + col * 4];
                }
                out.m[row + col * 4] = sum;
            }
        }
        return out;
    }
};

struct Vertex {
    Vec3 pos;
    Vec3 normal;
    Vec4 color;
    bool selected = false;
};

struct Edge {
    uint16_t v0, v1;
    bool selected = false;
};

struct Face {
    uint16_t v0, v1, v2;
    bool selected = false;
};

enum ObjectType {
    OBJ_MESH,
    OBJ_CAMERA,
    OBJ_LIGHT
};

enum ShadingMode {
    SHADE_WIREFRAME = 0,
    SHADE_SOLID = 1,
    SHADE_MATERIAL = 2,
    SHADE_RENDERED = 3
};

enum SolidLighting {
    LIGHT_STUDIO = 0,
    LIGHT_MATCAP = 1,
    LIGHT_FLAT = 2
};

enum MatCapType {
    MATCAP_CLAY = 0,
    MATCAP_RED_WAX = 1,
    MATCAP_CHROME = 2,
    MATCAP_PEARL = 3,
    MATCAP_NORMAL = 4
};

enum EditMode {
    MODE_OBJECT = 0,
    MODE_EDIT_VERTEX = 1,
    MODE_EDIT_EDGE = 2,
    MODE_EDIT_FACE = 3,
    MODE_SCULPT = 4
};

enum SculptBrush {
    BRUSH_DRAW = 0,
    BRUSH_CLAY = 1,
    BRUSH_SMOOTH = 2,
    BRUSH_FLATTEN = 3,
    BRUSH_INFLATE = 4,
    BRUSH_GRAB = 5
};

struct SculptSettings {
    SculptBrush activeBrush = BRUSH_DRAW;
    float brushRadius = 0.45f;
    float brushStrength = 0.50f;
    bool symmetryX = true;
};

// Blender 4.0+ Overhauled Principled BSDF v2
struct Material {
    std::string name = "Material";
    Vec4 baseColor = Vec4(0.80f, 0.82f, 0.85f, 1.0f);
    float subsurfaceWeight = 0.0f;
    Vec3 subsurfaceRadius = Vec3(1.0f, 0.2f, 0.1f);
    float subsurfaceScale = 0.1f;
    float metallic = 0.0f;
    float roughness = 0.45f;
    float ior = 1.45f;
    float specular = 0.50f;
    Vec4 specularTint = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float coatWeight = 0.0f;
    float coatRoughness = 0.03f;
    float sheenWeight = 0.0f;
    float sheenRoughness = 0.5f;
    Vec4 emissionColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    float emissionStrength = 1.0f;
    float alpha = 1.0f;
};

enum ModifierType {
    MOD_SUBSURF = 0,
    MOD_MIRROR = 1,
    MOD_BEVEL = 2,
    MOD_SOLIDIFY = 3,
    MOD_WIREFRAME = 4
};

struct Modifier {
    std::string name;
    ModifierType type;
    bool enabled = true;
    int subsurfLevels = 1;
    bool mirrorX = true;
    bool mirrorY = false;
    bool mirrorZ = false;
    float bevelWidth = 0.1f;
    int bevelSegments = 2;
    float solidifyThickness = 0.05f;
};

struct Keyframe {
    int frame = 1;
    Vec3 location;
    Vec3 rotation;
    Vec3 scale;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Edge> edges;
    std::vector<Face> faces;

    void recalculateNormals() {
        for (auto& v : vertices) v.normal = Vec3(0, 0, 0);
        for (const auto& f : faces) {
            if (f.v0 < vertices.size() && f.v1 < vertices.size() && f.v2 < vertices.size()) {
                Vec3 p0 = vertices[f.v0].pos;
                Vec3 p1 = vertices[f.v1].pos;
                Vec3 p2 = vertices[f.v2].pos;
                Vec3 fn = Vec3::cross(p1 - p0, p2 - p0);
                vertices[f.v0].normal = vertices[f.v0].normal + fn;
                vertices[f.v1].normal = vertices[f.v1].normal + fn;
                vertices[f.v2].normal = vertices[f.v2].normal + fn;
            }
        }
        for (auto& v : vertices) v.normal = v.normal.normalized();
    }
};

struct SceneObject {
    std::string name;
    ObjectType type = OBJ_MESH;
    Vec3 location = Vec3(0, 0, 0);
    Vec3 rotation = Vec3(0, 0, 0); // degrees
    Vec3 scale = Vec3(1, 1, 1);
    Mesh mesh;
    Material material;
    std::vector<Modifier> modifiers;
    std::vector<Keyframe> keyframes;
    bool selected = false;
    bool visible = true;
    bool renderVisible = true;
    bool selectable = true;

    Mat4 getTransformMatrix() const {
        float deg2rad = 3.14159265f / 180.0f;
        Mat4 T = Mat4::translation(location);
        Mat4 Rz = Mat4::rotationZ(rotation.z * deg2rad);
        Mat4 Ry = Mat4::rotationY(rotation.y * deg2rad);
        Mat4 Rx = Mat4::rotationX(rotation.x * deg2rad);
        Mat4 S = Mat4::scaling(scale);
        return T * Rz * Ry * Rx * S;
    }
};

struct Scene {
    std::string name = "Scene";
    std::vector<SceneObject> objects;
    int selectedIndex = 0;
    EditMode editMode = MODE_OBJECT;
    ShadingMode shadingMode = SHADE_SOLID;
    SolidLighting solidLighting = LIGHT_STUDIO;
    MatCapType matcapType = MATCAP_CLAY;
    bool showCavity = true;
    bool showXRay = false;
    bool showWireOverlay = false;
    bool showFloorGrid = true;
    bool showAxes = true;
    bool show3DCursor = true;

    SculptSettings sculpt;
    Vec3 cursor3D = Vec3(0, 0, 0);
    int currentFrame = 1;
    int totalFrames = 250;
    int fps = 24;
    bool isPlaying = false;

    SceneObject* getSelectedObject() {
        if (selectedIndex >= 0 && selectedIndex < (int)objects.size()) {
            return &objects[selectedIndex];
        }
        return nullptr;
    }

    void selectObject(int idx) {
        for (size_t i = 0; i < objects.size(); ++i) {
            objects[i].selected = ((int)i == idx);
        }
        selectedIndex = idx;
    }
};
