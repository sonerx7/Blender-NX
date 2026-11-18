#pragma once
#include "Scene.h"
#include "Suzanne.h"

namespace Primitives {

inline Mesh createCube(float size = 2.0f) {
    Mesh m;
    float h = size * 0.5f;

    // 8 cube corners
    Vec3 p[8] = {
        {-h, -h, -h}, { h, -h, -h}, { h,  h, -h}, {-h,  h, -h},
        {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h}
    };

    // 6 faces (each 2 triangles = 12 triangles)
    int faces[6][4] = {
        {0, 1, 2, 3}, // Back
        {5, 4, 7, 6}, // Front
        {4, 0, 3, 7}, // Left
        {1, 5, 6, 2}, // Right
        {4, 5, 1, 0}, // Bottom
        {3, 2, 6, 7}  // Top
    };

    for (int i = 0; i < 6; ++i) {
        int v0 = faces[i][0];
        int v1 = faces[i][1];
        int v2 = faces[i][2];
        int v3 = faces[i][3];

        uint16_t base = (uint16_t)m.vertices.size();
        m.vertices.push_back({p[v0], {0,0,0}, {0.8f, 0.8f, 0.85f, 1.0f}});
        m.vertices.push_back({p[v1], {0,0,0}, {0.8f, 0.8f, 0.85f, 1.0f}});
        m.vertices.push_back({p[v2], {0,0,0}, {0.8f, 0.8f, 0.85f, 1.0f}});
        m.vertices.push_back({p[v3], {0,0,0}, {0.8f, 0.8f, 0.85f, 1.0f}});

        m.faces.push_back({(uint16_t)(base + 0), (uint16_t)(base + 1), (uint16_t)(base + 2)});
        m.faces.push_back({(uint16_t)(base + 0), (uint16_t)(base + 2), (uint16_t)(base + 3)});

        m.edges.push_back({(uint16_t)(base + 0), (uint16_t)(base + 1)});
        m.edges.push_back({(uint16_t)(base + 1), (uint16_t)(base + 2)});
        m.edges.push_back({(uint16_t)(base + 2), (uint16_t)(base + 3)});
        m.edges.push_back({(uint16_t)(base + 3), (uint16_t)(base + 0)});
    }

    m.recalculateNormals();
    return m;
}

inline Mesh createSuzanne() {
    Mesh m;
    m.vertices.resize(kSuzanneVertexCount);
    for (int i = 0; i < kSuzanneVertexCount; ++i) {
        m.vertices[i].pos = Vec3(kSuzanneVertices[i][0], kSuzanneVertices[i][1], kSuzanneVertices[i][2]);
        m.vertices[i].normal = Vec3(kSuzanneNormals[i][0], kSuzanneNormals[i][1], kSuzanneNormals[i][2]);
        m.vertices[i].color = Vec4(0.85f, 0.75f, 0.65f, 1.0f);
    }

    m.faces.resize(kSuzanneTriangleCount);
    for (int i = 0; i < kSuzanneTriangleCount; ++i) {
        m.faces[i].v0 = kSuzanneIndices[i][0];
        m.faces[i].v1 = kSuzanneIndices[i][1];
        m.faces[i].v2 = kSuzanneIndices[i][2];

        m.edges.push_back({m.faces[i].v0, m.faces[i].v1});
        m.edges.push_back({m.faces[i].v1, m.faces[i].v2});
        m.edges.push_back({m.faces[i].v2, m.faces[i].v0});
    }

    return m;
}

inline Mesh createUVSphere(float radius = 1.0f, int rings = 14, int sectors = 20) {
    Mesh m;
    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);
    float const pi = 3.14159265f;

    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            float const y = std::sin(-pi * 0.5f + pi * r * R);
            float const x = std::cos(2.0f * pi * s * S) * std::sin(pi * r * R);
            float const z = std::sin(2.0f * pi * s * S) * std::sin(pi * r * R);

            Vec3 pos(x * radius, y * radius, z * radius);
            Vec3 norm(x, y, z);
            m.vertices.push_back({pos, norm, Vec4(0.8f, 0.8f, 0.85f, 1.0f)});
        }
    }

    for (int r = 0; r < rings - 1; ++r) {
        for (int s = 0; s < sectors - 1; ++s) {
            uint16_t cur0 = (uint16_t)(r * sectors + s);
            uint16_t cur1 = (uint16_t)(r * sectors + (s + 1));
            uint16_t next0 = (uint16_t)((r + 1) * sectors + s);
            uint16_t next1 = (uint16_t)((r + 1) * sectors + (s + 1));

            m.faces.push_back({cur0, cur1, next1});
            m.faces.push_back({cur0, next1, next0});

            m.edges.push_back({cur0, cur1});
            m.edges.push_back({cur0, next0});
        }
    }

    return m;
}

inline Mesh createIcoSphere(float radius = 1.0f) {
    Mesh m;
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    // 12 vertices of an icosahedron
    Vec3 pts[12] = {
        {-1,  t,  0}, { 1,  t,  0}, {-1, -t,  0}, { 1, -t,  0},
        { 0, -1,  t}, { 0,  1,  t}, { 0, -1, -t}, { 0,  1, -t},
        { t,  0, -1}, { t,  0,  1}, {-t,  0, -1}, {-t,  0,  1}
    };

    for (int i = 0; i < 12; ++i) {
        Vec3 n = pts[i].normalized();
        m.vertices.push_back({n * radius, n, Vec4(0.8f, 0.82f, 0.88f, 1.0f)});
    }

    // 20 triangular faces
    int faces[20][3] = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };

    for (int i = 0; i < 20; ++i) {
        uint16_t v0 = (uint16_t)faces[i][0];
        uint16_t v1 = (uint16_t)faces[i][1];
        uint16_t v2 = (uint16_t)faces[i][2];
        m.faces.push_back({v0, v1, v2});
        m.edges.push_back({v0, v1});
        m.edges.push_back({v1, v2});
        m.edges.push_back({v2, v0});
    }

    m.recalculateNormals();
    return m;
}

inline Mesh createCylinder(float radius = 1.0f, float height = 2.0f, int sectors = 16) {
    Mesh m;
    float const pi = 3.14159265f;
    float h = height * 0.5f;

    uint16_t topCenter = 0;
    uint16_t botCenter = 1;
    m.vertices.push_back({{0, h, 0}, {0, 1, 0}, {0.8f, 0.8f, 0.85f, 1.0f}});
    m.vertices.push_back({{0, -h, 0}, {0, -1, 0}, {0.8f, 0.8f, 0.85f, 1.0f}});

    uint16_t base = 2;
    for (int i = 0; i < sectors; ++i) {
        float angle = 2.0f * pi * i / sectors;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;

        m.vertices.push_back({{x, h, z}, Vec3(x, 0, z).normalized(), {0.8f, 0.8f, 0.85f, 1.0f}});
        m.vertices.push_back({{x, -h, z}, Vec3(x, 0, z).normalized(), {0.8f, 0.8f, 0.85f, 1.0f}});
    }

    for (int i = 0; i < sectors; ++i) {
        int next = (i + 1) % sectors;
        uint16_t top0 = base + i * 2;
        uint16_t bot0 = base + i * 2 + 1;
        uint16_t top1 = base + next * 2;
        uint16_t bot1 = base + next * 2 + 1;

        m.faces.push_back({top0, top1, bot1});
        m.faces.push_back({top0, bot1, bot0});

        m.faces.push_back({topCenter, top1, top0});
        m.faces.push_back({botCenter, bot0, bot1});

        m.edges.push_back({top0, top1});
        m.edges.push_back({bot0, bot1});
        m.edges.push_back({top0, bot0});
    }

    return m;
}

inline Mesh createCone(float radius = 1.0f, float height = 2.0f, int sectors = 16) {
    Mesh m;
    float const pi = 3.14159265f;
    float h = height * 0.5f;

    uint16_t apex = 0;
    uint16_t botCenter = 1;
    m.vertices.push_back({{0, h, 0}, {0, 1, 0}, {0.8f, 0.8f, 0.85f, 1.0f}});
    m.vertices.push_back({{0, -h, 0}, {0, -1, 0}, {0.8f, 0.8f, 0.85f, 1.0f}});

    uint16_t base = 2;
    for (int i = 0; i < sectors; ++i) {
        float angle = 2.0f * pi * i / sectors;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        m.vertices.push_back({{x, -h, z}, Vec3(x, 0.5f, z).normalized(), {0.8f, 0.8f, 0.85f, 1.0f}});
    }

    for (int i = 0; i < sectors; ++i) {
        int next = (i + 1) % sectors;
        uint16_t v0 = base + i;
        uint16_t v1 = base + next;

        // Side cone triangle
        m.faces.push_back({apex, v0, v1});
        // Bottom disk triangle
        m.faces.push_back({botCenter, v1, v0});

        m.edges.push_back({apex, v0});
        m.edges.push_back({v0, v1});
        m.edges.push_back({botCenter, v0});
    }

    return m;
}

inline Mesh createTorus(float majorR = 1.2f, float minorR = 0.35f, int majorSegs = 20, int minorSegs = 12) {
    Mesh m;
    float const pi = 3.14159265f;

    for (int i = 0; i < majorSegs; ++i) {
        float u = 2.0f * pi * i / majorSegs;
        float cosU = std::cos(u);
        float sinU = std::sin(u);

        for (int j = 0; j < minorSegs; ++j) {
            float v = 2.0f * pi * j / minorSegs;
            float cosV = std::cos(v);
            float sinV = std::sin(v);

            float x = (majorR + minorR * cosV) * cosU;
            float y = minorR * sinV;
            float z = (majorR + minorR * cosV) * sinU;

            Vec3 norm(cosV * cosU, sinV, cosV * sinU);
            m.vertices.push_back({{x, y, z}, norm.normalized(), {0.8f, 0.8f, 0.85f, 1.0f}});
        }
    }

    for (int i = 0; i < majorSegs; ++i) {
        int nextI = (i + 1) % majorSegs;
        for (int j = 0; j < minorSegs; ++j) {
            int nextJ = (j + 1) % minorSegs;

            uint16_t v00 = (uint16_t)(i * minorSegs + j);
            uint16_t v10 = (uint16_t)(nextI * minorSegs + j);
            uint16_t v01 = (uint16_t)(i * minorSegs + nextJ);
            uint16_t v11 = (uint16_t)(nextI * minorSegs + nextJ);

            m.faces.push_back({v00, v10, v11});
            m.faces.push_back({v00, v11, v01});

            m.edges.push_back({v00, v10});
            m.edges.push_back({v00, v01});
        }
    }

    return m;
}

inline Mesh createPlane(float size = 4.0f) {
    Mesh m;
    float h = size * 0.5f;
    m.vertices.push_back({{-h, 0, -h}, {0, 1, 0}, {0.8f, 0.8f, 0.85f, 1.0f}});
    m.vertices.push_back({{ h, 0, -h}, {0, 1, 0}, {0.8f, 0.8f, 0.85f, 1.0f}});
    m.vertices.push_back({{ h, 0,  h}, {0, 1, 0}, {0.8f, 0.8f, 0.85f, 1.0f}});
    m.vertices.push_back({{-h, 0,  h}, {0, 1, 0}, {0.8f, 0.8f, 0.85f, 1.0f}});

    m.faces.push_back({0, 1, 2});
    m.faces.push_back({0, 2, 3});

    m.edges.push_back({0, 1});
    m.edges.push_back({1, 2});
    m.edges.push_back({2, 3});
    m.edges.push_back({3, 0});

    return m;
}

inline Mesh createCameraWireframe() {
    Mesh m;
    m.vertices.push_back({{0, 0, 0}, {0, 0, 1}, {1.0f, 0.6f, 0.1f, 1.0f}});
    m.vertices.push_back({{-0.6f, -0.4f, -1.0f}, {0, 0, 1}, {1.0f, 0.6f, 0.1f, 1.0f}});
    m.vertices.push_back({{ 0.6f, -0.4f, -1.0f}, {0, 0, 1}, {1.0f, 0.6f, 0.1f, 1.0f}});
    m.vertices.push_back({{ 0.6f,  0.4f, -1.0f}, {0, 0, 1}, {1.0f, 0.6f, 0.1f, 1.0f}});
    m.vertices.push_back({{-0.6f,  0.4f, -1.0f}, {0, 0, 1}, {1.0f, 0.6f, 0.1f, 1.0f}});
    m.vertices.push_back({{0.0f, 0.6f, -1.0f}, {0, 0, 1}, {1.0f, 0.6f, 0.1f, 1.0f}});

    m.edges.push_back({0, 1}); m.edges.push_back({0, 2});
    m.edges.push_back({0, 3}); m.edges.push_back({0, 4});
    m.edges.push_back({1, 2}); m.edges.push_back({2, 3});
    m.edges.push_back({3, 4}); m.edges.push_back({4, 1});
    m.edges.push_back({4, 5}); m.edges.push_back({3, 5});

    return m;
}

inline Mesh createLightWireframe() {
    Mesh m;
    float r = 0.5f;
    float pi = 3.14159265f;
    int n = 12;
    for (int i = 0; i < n; ++i) {
        float a = 2.0f * pi * i / n;
        m.vertices.push_back({{std::cos(a)*r, std::sin(a)*r, 0}, {0,0,1}, {1.0f, 0.9f, 0.2f, 1.0f}});
    }
    for (int i = 0; i < n; ++i) {
        m.edges.push_back({(uint16_t)i, (uint16_t)((i+1)%n)});
    }
    return m;
}

} // namespace Primitives
