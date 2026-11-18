#pragma once
#include "Scene.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace BlenderIO {

inline bool exportOBJ(const Scene& scene, const std::string& path) {
    std::ofstream out(path);
    if (!out.is_open()) return false;

    out << "# Blender 4.2 LTS NX - Wavefront OBJ Exporter\n";
    out << "# Exported Scene: " << scene.name << "\n\n";

    uint16_t vertexOffset = 1;

    for (const auto& obj : scene.objects) {
        if (!obj.visible || obj.mesh.vertices.empty()) continue;

        out << "o " << obj.name << "\n";
        Mat4 model = obj.getTransformMatrix();

        // Vertices (world coordinates)
        for (const auto& v : obj.mesh.vertices) {
            float x = model.m[0]*v.pos.x + model.m[4]*v.pos.y + model.m[8]*v.pos.z + model.m[12];
            float y = model.m[1]*v.pos.x + model.m[5]*v.pos.y + model.m[9]*v.pos.z + model.m[13];
            float z = model.m[2]*v.pos.x + model.m[6]*v.pos.y + model.m[10]*v.pos.z + model.m[14];
            out << "v " << std::fixed << std::setprecision(6) << x << " " << y << " " << z << "\n";
        }

        // Normals
        for (const auto& v : obj.mesh.vertices) {
            float nx = model.m[0]*v.normal.x + model.m[4]*v.normal.y + model.m[8]*v.normal.z;
            float ny = model.m[1]*v.normal.x + model.m[5]*v.normal.y + model.m[9]*v.normal.z;
            float nz = model.m[2]*v.normal.x + model.m[6]*v.normal.y + model.m[10]*v.normal.z;
            Vec3 n = Vec3(nx, ny, nz).normalized();
            out << "vn " << std::fixed << std::setprecision(6) << n.x << " " << n.y << " " << n.z << "\n";
        }

        // Faces
        for (const auto& f : obj.mesh.faces) {
            uint16_t i0 = f.v0 + vertexOffset;
            uint16_t i1 = f.v1 + vertexOffset;
            uint16_t i2 = f.v2 + vertexOffset;
            out << "f " << i0 << "//" << i0 << " " << i1 << "//" << i1 << " " << i2 << "//" << i2 << "\n";
        }

        vertexOffset += (uint16_t)obj.mesh.vertices.size();
        out << "\n";
    }

    return true;
}

inline bool exportGLTF(const Scene& scene, const std::string& path) {
    std::ofstream out(path);
    if (!out.is_open()) return false;

    out << "{\n";
    out << "  \"asset\": {\n";
    out << "    \"version\": \"2.0\",\n";
    out << "    \"generator\": \"Blender 4.2 LTS NX\"\n";
    out << "  },\n";
    out << "  \"scenes\": [ { \"name\": \"" << scene.name << "\", \"nodes\": [";
    for (size_t i = 0; i < scene.objects.size(); ++i) {
        out << i << (i + 1 < scene.objects.size() ? ", " : "");
    }
    out << "] } ],\n";
    out << "  \"scene\": 0,\n";
    out << "  \"nodes\": [\n";

    for (size_t i = 0; i < scene.objects.size(); ++i) {
        const auto& obj = scene.objects[i];
        out << "    {\n";
        out << "      \"name\": \"" << obj.name << "\",\n";
        out << "      \"translation\": [" << obj.location.x << ", " << obj.location.y << ", " << obj.location.z << "],\n";
        out << "      \"scale\": [" << obj.scale.x << ", " << obj.scale.y << ", " << obj.scale.z << "]\n";
        out << "    }" << (i + 1 < scene.objects.size() ? ",\n" : "\n");
    }

    out << "  ]\n";
    out << "}\n";
    return true;
}

inline bool exportSTL(const Scene& scene, const std::string& path) {
    std::ofstream out(path);
    if (!out.is_open()) return false;

    out << "solid " << scene.name << "\n";
    for (const auto& obj : scene.objects) {
        if (!obj.visible || obj.mesh.vertices.empty()) continue;
        Mat4 model = obj.getTransformMatrix();

        auto transformPt = [&](const Vec3& p) -> Vec3 {
            return Vec3(
                model.m[0]*p.x + model.m[4]*p.y + model.m[8]*p.z  + model.m[12],
                model.m[1]*p.x + model.m[5]*p.y + model.m[9]*p.z  + model.m[13],
                model.m[2]*p.x + model.m[6]*p.y + model.m[10]*p.z + model.m[14]
            );
        };

        for (const auto& f : obj.mesh.faces) {
            Vec3 p0 = transformPt(obj.mesh.vertices[f.v0].pos);
            Vec3 p1 = transformPt(obj.mesh.vertices[f.v1].pos);
            Vec3 p2 = transformPt(obj.mesh.vertices[f.v2].pos);
            Vec3 n = Vec3::cross(p1 - p0, p2 - p0).normalized();

            out << "  facet normal " << n.x << " " << n.y << " " << n.z << "\n";
            out << "    outer loop\n";
            out << "      vertex " << p0.x << " " << p0.y << " " << p0.z << "\n";
            out << "      vertex " << p1.x << " " << p1.y << " " << p1.z << "\n";
            out << "      vertex " << p2.x << " " << p2.y << " " << p2.z << "\n";
            out << "    endloop\n";
            out << "  endfacet\n";
        }
    }
    out << "endsolid " << scene.name << "\n";
    return true;
}

inline bool saveBlendFile(const Scene& scene, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;

    // Standard Blender .blend header: "BLENDER-v420" (12 bytes for Blender 4.2 LTS)
    const char header[12] = {'B', 'L', 'E', 'N', 'D', 'E', 'R', '-', 'v', '4', '2', '0'};
    out.write(header, 12);

    uint32_t numObjs = (uint32_t)scene.objects.size();
    out.write((const char*)&numObjs, sizeof(numObjs));

    for (const auto& obj : scene.objects) {
        uint32_t nameLen = (uint32_t)obj.name.size();
        out.write((const char*)&nameLen, sizeof(nameLen));
        out.write(obj.name.data(), nameLen);

        out.write((const char*)&obj.location, sizeof(Vec3));
        out.write((const char*)&obj.rotation, sizeof(Vec3));
        out.write((const char*)&obj.scale, sizeof(Vec3));

        uint32_t vertCount = (uint32_t)obj.mesh.vertices.size();
        out.write((const char*)&vertCount, sizeof(vertCount));
        for (const auto& v : obj.mesh.vertices) {
            out.write((const char*)&v.pos, sizeof(Vec3));
            out.write((const char*)&v.normal, sizeof(Vec3));
        }

        uint32_t faceCount = (uint32_t)obj.mesh.faces.size();
        out.write((const char*)&faceCount, sizeof(faceCount));
        for (const auto& f : obj.mesh.faces) {
            out.write((const char*)&f.v0, sizeof(uint16_t));
            out.write((const char*)&f.v1, sizeof(uint16_t));
            out.write((const char*)&f.v2, sizeof(uint16_t));
        }
    }

    return true;
}

} // namespace BlenderIO
