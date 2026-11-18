#pragma once
#include "Scene.h"
#include <map>
#include <algorithm>
#include <cmath>

namespace MeshOps {

inline void subdivide(Mesh& mesh) {
    if (mesh.faces.empty()) return;

    std::map<uint32_t, uint16_t> midpointCache;
    auto getMidpoint = [&](uint16_t v0, uint16_t v1) -> uint16_t {
        uint32_t key = (v0 < v1) ? (((uint32_t)v0 << 16) | v1) : (((uint32_t)v1 << 16) | v0);
        auto it = midpointCache.find(key);
        if (it != midpointCache.end()) return it->second;

        Vertex m;
        m.pos = (mesh.vertices[v0].pos + mesh.vertices[v1].pos) * 0.5f;
        m.normal = (mesh.vertices[v0].normal + mesh.vertices[v1].normal).normalized();
        m.color = mesh.vertices[v0].color;
        uint16_t newIdx = (uint16_t)mesh.vertices.size();
        mesh.vertices.push_back(m);
        midpointCache[key] = newIdx;
        return newIdx;
    };

    std::vector<Face> newFaces;
    std::vector<Edge> newEdges;

    for (const auto& f : mesh.faces) {
        uint16_t m01 = getMidpoint(f.v0, f.v1);
        uint16_t m12 = getMidpoint(f.v1, f.v2);
        uint16_t m20 = getMidpoint(f.v2, f.v0);

        // 4 new sub-triangles
        newFaces.push_back({f.v0, m01, m20});
        newFaces.push_back({f.v1, m12, m01});
        newFaces.push_back({f.v2, m20, m12});
        newFaces.push_back({m01, m12, m20});

        newEdges.push_back({f.v0, m01}); newEdges.push_back({m01, f.v1});
        newEdges.push_back({f.v1, m12}); newEdges.push_back({m12, f.v2});
        newEdges.push_back({f.v2, m20}); newEdges.push_back({m20, f.v0});
        newEdges.push_back({m01, m12}); newEdges.push_back({m12, m20}); newEdges.push_back({m20, m01});
    }

    mesh.faces = newFaces;
    mesh.edges = newEdges;
    mesh.recalculateNormals();
}

inline void extrudeFace(Mesh& mesh, int faceIdx, float dist) {
    if (faceIdx < 0 || faceIdx >= (int)mesh.faces.size()) return;

    Face origF = mesh.faces[faceIdx];
    Vec3 fn = Vec3::cross(mesh.vertices[origF.v1].pos - mesh.vertices[origF.v0].pos,
                          mesh.vertices[origF.v2].pos - mesh.vertices[origF.v0].pos).normalized();

    uint16_t nv0 = (uint16_t)mesh.vertices.size();
    mesh.vertices.push_back({mesh.vertices[origF.v0].pos + fn * dist, fn, mesh.vertices[origF.v0].color});
    uint16_t nv1 = (uint16_t)mesh.vertices.size();
    mesh.vertices.push_back({mesh.vertices[origF.v1].pos + fn * dist, fn, mesh.vertices[origF.v1].color});
    uint16_t nv2 = (uint16_t)mesh.vertices.size();
    mesh.vertices.push_back({mesh.vertices[origF.v2].pos + fn * dist, fn, mesh.vertices[origF.v2].color});

    mesh.faces[faceIdx] = {nv0, nv1, nv2};

    auto addSide = [&](uint16_t b0, uint16_t b1, uint16_t t0, uint16_t t1) {
        mesh.faces.push_back({b0, b1, t1});
        mesh.faces.push_back({b0, t1, t0});
        mesh.edges.push_back({b0, t0});
        mesh.edges.push_back({b1, t1});
        mesh.edges.push_back({t0, t1});
    };

    addSide(origF.v0, origF.v1, nv0, nv1);
    addSide(origF.v1, origF.v2, nv1, nv2);
    addSide(origF.v2, origF.v0, nv2, nv0);

    mesh.recalculateNormals();
}

inline void insetFace(Mesh& mesh, int faceIdx, float amount = 0.2f) {
    if (faceIdx < 0 || faceIdx >= (int)mesh.faces.size()) return;

    Face origF = mesh.faces[faceIdx];
    Vec3 c = (mesh.vertices[origF.v0].pos + mesh.vertices[origF.v1].pos + mesh.vertices[origF.v2].pos) / 3.0f;
    Vec3 fn = mesh.vertices[origF.v0].normal;

    uint16_t nv0 = (uint16_t)mesh.vertices.size();
    mesh.vertices.push_back({mesh.vertices[origF.v0].pos * (1.0f - amount) + c * amount, fn, mesh.vertices[origF.v0].color});
    uint16_t nv1 = (uint16_t)mesh.vertices.size();
    mesh.vertices.push_back({mesh.vertices[origF.v1].pos * (1.0f - amount) + c * amount, fn, mesh.vertices[origF.v1].color});
    uint16_t nv2 = (uint16_t)mesh.vertices.size();
    mesh.vertices.push_back({mesh.vertices[origF.v2].pos * (1.0f - amount) + c * amount, fn, mesh.vertices[origF.v2].color});

    mesh.faces[faceIdx] = {nv0, nv1, nv2};

    mesh.faces.push_back({origF.v0, origF.v1, nv1});
    mesh.faces.push_back({origF.v0, nv1, nv0});
    mesh.faces.push_back({origF.v1, origF.v2, nv2});
    mesh.faces.push_back({origF.v1, nv2, nv1});
    mesh.faces.push_back({origF.v2, origF.v0, nv0});
    mesh.faces.push_back({origF.v2, nv0, nv2});

    mesh.recalculateNormals();
}

inline void applyMirror(Mesh& mesh, bool mirrorX, bool mirrorY, bool mirrorZ) {
    if (!mirrorX && !mirrorY && !mirrorZ) return;

    size_t origCount = mesh.vertices.size();
    size_t origFaces = mesh.faces.size();

    for (size_t i = 0; i < origCount; ++i) {
        Vertex v = mesh.vertices[i];
        if (mirrorX) { v.pos.x = -v.pos.x; v.normal.x = -v.normal.x; }
        if (mirrorY) { v.pos.y = -v.pos.y; v.normal.y = -v.normal.y; }
        if (mirrorZ) { v.pos.z = -v.pos.z; v.normal.z = -v.normal.z; }
        mesh.vertices.push_back(v);
    }

    uint16_t offset = (uint16_t)origCount;
    for (size_t i = 0; i < origFaces; ++i) {
        Face f = mesh.faces[i];
        // Invert winding order so normals point outside
        mesh.faces.push_back({(uint16_t)(f.v0 + offset), (uint16_t)(f.v2 + offset), (uint16_t)(f.v1 + offset)});
    }

    mesh.recalculateNormals();
}

inline void applySculptBrush(Mesh& mesh, const Vec3& hitPos, const Vec3& hitNormal, SculptBrush brush, float radius, float strength, bool symmetryX) {
    if (mesh.vertices.empty()) return;

    auto deformPoint = [&](const Vec3& center, const Vec3& norm) {
        for (auto& v : mesh.vertices) {
            float dist = (v.pos - center).length();
            if (dist < radius) {
                float t = dist / radius;
                float falloff = (1.0f - t * t) * (1.0f - t * t) * strength * 0.15f;

                switch (brush) {
                    case BRUSH_DRAW:
                    case BRUSH_CLAY:
                        v.pos = v.pos + norm * falloff;
                        break;
                    case BRUSH_INFLATE:
                        v.pos = v.pos + v.normal * falloff;
                        break;
                    case BRUSH_SMOOTH:
                        v.pos = v.pos + (center - v.pos) * (falloff * 0.5f);
                        break;
                    case BRUSH_FLATTEN: {
                        float planeDist = Vec3::dot(v.pos - center, norm);
                        v.pos = v.pos - norm * (planeDist * falloff);
                        break;
                    }
                    case BRUSH_GRAB:
                        v.pos = v.pos + norm * falloff;
                        break;
                }
            }
        }
    };

    deformPoint(hitPos, hitNormal);

    if (symmetryX) {
        Vec3 symCenter(-hitPos.x, hitPos.y, hitPos.z);
        Vec3 symNorm(-hitNormal.x, hitNormal.y, hitNormal.z);
        deformPoint(symCenter, symNorm);
    }

    mesh.recalculateNormals();
}

} // namespace MeshOps
