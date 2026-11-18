#include "Viewport3D.h"
#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof log, nullptr, log);
        std::fprintf(stderr, "Shader compile error (%d): %s\n", type, log);
    }
    return s;
}

static GLuint createProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

Viewport3D::Viewport3D() {}

Viewport3D::~Viewport3D() {
    shutdown();
}

bool Viewport3D::init() {
    initShaders();
    resize(m_width, m_height);
    return true;
}

void Viewport3D::shutdown() {
    if (m_fbo) { glDeleteFramebuffers(1, &m_fbo); m_fbo = 0; }
    if (m_colorTex) { glDeleteTextures(1, &m_colorTex); m_colorTex = 0; }
    if (m_depthRbo) { glDeleteRenderbuffers(1, &m_depthRbo); m_depthRbo = 0; }
    if (m_progShaded) { glDeleteProgram(m_progShaded); m_progShaded = 0; }
    if (m_progFlat) { glDeleteProgram(m_progFlat); m_progFlat = 0; }
}

void Viewport3D::initShaders() {
    // Blender 4.2 LTS Unified Shaded Vertex Shader
    const char* vsShaded =
        "#version 330 core\n"
        "layout(location = 0) in vec3 inPos;\n"
        "layout(location = 1) in vec3 inNormal;\n"
        "uniform mat4 uMVP;\n"
        "uniform mat4 uModel;\n"
        "uniform mat4 uView;\n"
        "out vec3 vWorldNormal;\n"
        "out vec3 vWorldPos;\n"
        "out vec3 vViewNormal;\n"
        "void main() {\n"
        "    vWorldPos = vec3(uModel * vec4(inPos, 1.0));\n"
        "    vWorldNormal = normalize(mat3(uModel) * inNormal);\n"
        "    vViewNormal = normalize(mat3(uView) * vWorldNormal);\n"
        "    gl_Position = uMVP * vec4(inPos, 1.0);\n"
        "}\n";

    // Blender 4.2 LTS Unified Shaded Fragment Shader (Studio, MatCap, Material Preview & Cavity)
    const char* fsShaded =
        "#version 330 core\n"
        "in vec3 vWorldNormal;\n"
        "in vec3 vWorldPos;\n"
        "in vec3 vViewNormal;\n"
        "uniform vec4 uColor;\n"
        "uniform vec3 uLightDir;\n"
        "uniform float uRoughness;\n"
        "uniform float uMetallic;\n"
        "uniform float uSpecular;\n"
        "uniform int uShadingMode;\n"   // 1=Solid, 2=Material, 3=Rendered
        "uniform int uLightingType;\n"  // 0=Studio, 1=MatCap, 2=Flat
        "uniform int uMatCapType;\n"    // 0=Clay, 1=Red Wax, 2=Chrome, 3=Pearl, 4=Normal
        "uniform int uCavity;\n"        // 0=Off, 1=On
        "uniform float uXRay;\n"        // 0.0=Solid, >0.0=Alpha
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    vec3 N = normalize(vWorldNormal);\n"
        "    vec3 VN = normalize(vViewNormal);\n"
        "    vec3 col = uColor.rgb;\n"
        "\n"
        "    // MatCap Shading\n"
        "    if (uShadingMode == 1 && uLightingType == 1) {\n"
        "        if (uMatCapType == 0) {\n" // Clay
        "            float d = VN.z * 0.5 + 0.5;\n"
        "            float top = max(VN.y, 0.0) * 0.35;\n"
        "            col = vec3(0.78, 0.72, 0.68) * (d * 0.75 + 0.25) + top;\n"
        "        } else if (uMatCapType == 1) {\n" // Red Wax
        "            float rim = 1.0 - max(VN.z, 0.0);\n"
        "            float spec = pow(max(dot(VN, normalize(vec3(0.4, 0.6, 0.7))), 0.0), 16.0) * 0.6;\n"
        "            vec3 baseWax = vec3(0.75, 0.22, 0.18);\n"
        "            col = baseWax * (VN.z * 0.65 + 0.35) + vec3(0.9, 0.5, 0.4) * pow(rim, 3.0) * 0.4 + spec;\n"
        "        } else if (uMatCapType == 2) {\n" // Chrome
        "            float horiz = abs(VN.y);\n"
        "            float sky = step(0.0, VN.y);\n"
        "            vec3 groundCol = mix(vec3(0.2, 0.18, 0.16), vec3(0.6, 0.55, 0.5), horiz);\n"
        "            vec3 skyCol = mix(vec3(0.4, 0.6, 0.9), vec3(0.95, 0.98, 1.0), horiz);\n"
        "            col = mix(groundCol, skyCol, sky);\n"
        "            col += pow(max(VN.z, 0.0), 32.0) * 0.5;\n"
        "        } else if (uMatCapType == 3) {\n" // Pearl
        "            float d = VN.z * 0.5 + 0.5;\n"
        "            vec3 iridescent = 0.5 + 0.5 * cos(6.28318 * (VN.y * 0.5 + vec3(0.0, 0.33, 0.67)));\n"
        "            col = mix(vec3(0.85, 0.88, 0.92), iridescent, 0.25) * d;\n"
        "        } else {\n" // Normal
        "            col = VN * 0.5 + 0.5;\n"
        "        }\n"
        "    } else if (uLightingType == 2) {\n" // Flat
        "        col = uColor.rgb;\n"
        "    } else {\n" // Studio Lighting / Material Preview / Rendered
        "        vec3 L1 = normalize(uLightDir);\n"
        "        vec3 L2 = normalize(vec3(-uLightDir.x, 0.4, -uLightDir.z));\n"
        "        vec3 V = normalize(-vWorldPos);\n"
        "        vec3 H1 = normalize(L1 + V);\n"
        "        float diff1 = max(dot(N, L1), 0.0);\n"
        "        float diff2 = max(dot(N, L2), 0.0) * 0.32;\n"
        "        float spec = pow(max(dot(N, H1), 0.0), mix(64.0, 4.0, uRoughness)) * (1.0 - uRoughness) * uSpecular;\n"
        "        float ambient = 0.28;\n"
        "        vec3 base = (uShadingMode == 1) ? vec3(0.80, 0.82, 0.85) : uColor.rgb;\n"
        "        if (uMetallic > 0.0) {\n"
        "            base = mix(base, base * (diff1 + ambient), 1.0 - uMetallic);\n"
        "            spec *= (1.0 + uMetallic * 2.0);\n"
        "        }\n"
        "        col = (ambient + diff1 * 0.65 + diff2) * base + vec3(spec);\n"
        "    }\n"
        "\n"
        "    // Viewport Cavity / Ridge Accent\n"
        "    if (uCavity == 1) {\n"
        "        float curvature = length(fwidth(N)) * 1.5;\n"
        "        col = mix(col, col * 0.6, clamp(curvature, 0.0, 0.4));\n"
        "    }\n"
        "\n"
        "    float alpha = (uXRay > 0.0) ? uXRay : uColor.a;\n"
        "    FragColor = vec4(col, alpha);\n"
        "}\n";

    m_progShaded = createProgram(vsShaded, fsShaded);
    m_uMVP = glGetUniformLocation(m_progShaded, "uMVP");
    m_uModel = glGetUniformLocation(m_progShaded, "uModel");
    m_uView = glGetUniformLocation(m_progShaded, "uView");
    m_uColor = glGetUniformLocation(m_progShaded, "uColor");
    m_uLightDir = glGetUniformLocation(m_progShaded, "uLightDir");
    m_uRoughness = glGetUniformLocation(m_progShaded, "uRoughness");
    m_uMetallic = glGetUniformLocation(m_progShaded, "uMetallic");
    m_uSpecular = glGetUniformLocation(m_progShaded, "uSpecular");
    m_uShadingMode = glGetUniformLocation(m_progShaded, "uShadingMode");
    m_uLightingType = glGetUniformLocation(m_progShaded, "uLightingType");
    m_uMatCapType = glGetUniformLocation(m_progShaded, "uMatCapType");
    m_uCavity = glGetUniformLocation(m_progShaded, "uCavity");
    m_uXRay = glGetUniformLocation(m_progShaded, "uXRay");

    const char* vsFlat =
        "#version 330 core\n"
        "layout(location = 0) in vec3 inPos;\n"
        "uniform mat4 uMVP;\n"
        "void main() {\n"
        "    gl_Position = uMVP * vec4(inPos, 1.0);\n"
        "}\n";

    const char* fsFlat =
        "#version 330 core\n"
        "uniform vec4 uColor;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = uColor;\n"
        "}\n";

    m_progFlat = createProgram(vsFlat, fsFlat);
    m_uFlatMVP = glGetUniformLocation(m_progFlat, "uMVP");
    m_uFlatColor = glGetUniformLocation(m_progFlat, "uColor");
}

void Viewport3D::resize(int w, int h) {
    if (w < 64) w = 64;
    if (h < 64) h = 64;
    if (w == m_width && h == m_height && m_fbo != 0) return;

    m_width = w;
    m_height = h;

    if (!m_fbo) glGenFramebuffers(1, &m_fbo);
    if (!m_colorTex) glGenTextures(1, &m_colorTex);
    if (!m_depthRbo) glGenRenderbuffers(1, &m_depthRbo);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Vec3 Viewport3D::getCameraEye() const {
    float deg2rad = 3.14159265f / 180.0f;
    float radYaw = m_yaw * deg2rad;
    float radPitch = m_pitch * deg2rad;

    Vec3 eye;
    eye.x = m_target.x + m_distance * std::cos(radPitch) * std::sin(radYaw);
    eye.y = m_target.y + m_distance * std::sin(radPitch);
    eye.z = m_target.z + m_distance * std::cos(radPitch) * std::cos(radYaw);
    return eye;
}

void Viewport3D::orbit(float deltaYaw, float deltaPitch) {
    m_yaw += deltaYaw;
    m_pitch += deltaPitch;
    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;
}

void Viewport3D::pan(float deltaX, float deltaY) {
    float deg2rad = 3.14159265f / 180.0f;
    float radYaw = m_yaw * deg2rad;
    Vec3 right(std::cos(radYaw), 0, -std::sin(radYaw));
    Vec3 up(0, 1, 0);
    m_target = m_target + right * deltaX + up * deltaY;
}

void Viewport3D::zoom(float deltaDist) {
    m_distance += deltaDist;
    if (m_distance < 0.5f) m_distance = 0.5f;
    if (m_distance > 50.0f) m_distance = 50.0f;
}

void Viewport3D::resetView() {
    m_target = Vec3(0, 0, 0);
    m_yaw = 45.0f;
    m_pitch = 30.0f;
    m_distance = 6.0f;
}

void Viewport3D::setViewFront() {
    m_yaw = 0.0f;
    m_pitch = 0.0f;
}

void Viewport3D::setViewSide() {
    m_yaw = 90.0f;
    m_pitch = 0.0f;
}

void Viewport3D::setViewTop() {
    m_yaw = 0.0f;
    m_pitch = 89.9f;
}

void Viewport3D::renderGrid(const Mat4& vp, bool showFloor, bool showAxes) {
    if (!showFloor && !showAxes) return;

    glUseProgram(m_progFlat);

    if (showFloor) {
        std::vector<float> gridLines;
        int halfLines = 14;
        float step = 1.0f;

        for (int i = -halfLines; i <= halfLines; ++i) {
            float p = (float)i * step;
            gridLines.push_back(p); gridLines.push_back(0.0f); gridLines.push_back(-halfLines * step);
            gridLines.push_back(p); gridLines.push_back(0.0f); gridLines.push_back( halfLines * step);

            gridLines.push_back(-halfLines * step); gridLines.push_back(0.0f); gridLines.push_back(p);
            gridLines.push_back( halfLines * step); gridLines.push_back(0.0f); gridLines.push_back(p);
        }

        GLuint vao = 0, vbo = 0;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, gridLines.size() * sizeof(float), gridLines.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glUniformMatrix4fv(m_uFlatMVP, 1, GL_FALSE, vp.m);
        glUniform4f(m_uFlatColor, 0.23f, 0.23f, 0.25f, 1.0f);
        glDrawArrays(GL_LINES, 0, (GLsizei)(gridLines.size() / 3));

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    if (showAxes) {
        float axisData[12] = {
            -14.0f, 0.0f, 0.0f,   14.0f, 0.0f, 0.0f, // X (Red)
              0.0f, 0.0f, -14.0f,  0.0f, 0.0f, 14.0f  // Y/Z (Green)
        };
        GLuint vao = 0, vbo = 0;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(axisData), axisData, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glUniformMatrix4fv(m_uFlatMVP, 1, GL_FALSE, vp.m);
        glUniform4f(m_uFlatColor, 0.82f, 0.24f, 0.24f, 1.0f);
        glDrawArrays(GL_LINES, 0, 2);

        glUniform4f(m_uFlatColor, 0.32f, 0.78f, 0.24f, 1.0f);
        glDrawArrays(GL_LINES, 2, 2);

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
}

void Viewport3D::renderObject(const SceneObject& obj, const Mat4& vp, const Mat4& view, bool isSelected, const Scene& scene) {
    if (!obj.visible || obj.mesh.vertices.empty()) return;

    Mat4 model = obj.getTransformMatrix();
    Mat4 mvp = vp * model;

    std::vector<float> vboData;
    vboData.reserve(obj.mesh.vertices.size() * 6);
    for (const auto& v : obj.mesh.vertices) {
        vboData.push_back(v.pos.x);
        vboData.push_back(v.pos.y);
        vboData.push_back(v.pos.z);
        vboData.push_back(v.normal.x);
        vboData.push_back(v.normal.y);
        vboData.push_back(v.normal.z);
    }

    std::vector<uint16_t> indices;
    indices.reserve(obj.mesh.faces.size() * 3);
    for (const auto& f : obj.mesh.faces) {
        indices.push_back(f.v0);
        indices.push_back(f.v1);
        indices.push_back(f.v2);
    }

    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    if (!indices.empty()) glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(float), vboData.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    if (!indices.empty()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint16_t), indices.data(), GL_DYNAMIC_DRAW);
    }

    // 1. Shaded solid faces
    if (scene.shadingMode != SHADE_WIREFRAME && !indices.empty() && obj.type == OBJ_MESH) {
        glUseProgram(m_progShaded);
        glUniformMatrix4fv(m_uMVP, 1, GL_FALSE, mvp.m);
        glUniformMatrix4fv(m_uModel, 1, GL_FALSE, model.m);
        glUniformMatrix4fv(m_uView, 1, GL_FALSE, view.m);

        Vec4 col = (scene.shadingMode == SHADE_SOLID) ? Vec4(0.80f, 0.82f, 0.85f, 1.0f) : obj.material.baseColor;
        glUniform4f(m_uColor, col.x, col.y, col.z, col.w);
        glUniform3f(m_uLightDir, 0.5f, 0.8f, 1.0f);
        glUniform1f(m_uRoughness, obj.material.roughness);
        glUniform1f(m_uMetallic, obj.material.metallic);
        glUniform1f(m_uSpecular, obj.material.specular);
        glUniform1i(m_uShadingMode, (int)scene.shadingMode);
        glUniform1i(m_uLightingType, (int)scene.solidLighting);
        glUniform1i(m_uMatCapType, (int)scene.matcapType);
        glUniform1i(m_uCavity, scene.showCavity ? 1 : 0);
        glUniform1f(m_uXRay, scene.showXRay ? 0.45f : 0.0f);

        if (scene.showXRay) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glDisable(GL_BLEND);
        }

        glEnable(GL_DEPTH_TEST);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_SHORT, (void*)0);
    }

    // 2. Wireframe / Selection Outline / Camera / Light
    bool drawWire = (scene.shadingMode == SHADE_WIREFRAME) || isSelected || scene.showWireOverlay || (obj.type != OBJ_MESH);
    if (!obj.mesh.edges.empty() && drawWire) {
        glUseProgram(m_progFlat);
        glUniformMatrix4fv(m_uFlatMVP, 1, GL_FALSE, mvp.m);

        if (isSelected) {
            // Iconic Blender Vibrant Orange outline (#FF8500)
            glUniform4f(m_uFlatColor, 1.00f, 0.52f, 0.05f, 1.0f);
        } else if (obj.type == OBJ_CAMERA) {
            glUniform4f(m_uFlatColor, 1.0f, 0.6f, 0.2f, 1.0f);
        } else if (obj.type == OBJ_LIGHT) {
            glUniform4f(m_uFlatColor, 1.0f, 0.9f, 0.2f, 1.0f);
        } else {
            // Dark gray wireframe overlay
            glUniform4f(m_uFlatColor, 0.16f, 0.16f, 0.18f, 1.0f);
        }

        std::vector<uint16_t> edgeIndices;
        edgeIndices.reserve(obj.mesh.edges.size() * 2);
        for (const auto& e : obj.mesh.edges) {
            edgeIndices.push_back(e.v0);
            edgeIndices.push_back(e.v1);
        }

        GLuint edgeEbo = 0;
        glGenBuffers(1, &edgeEbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, edgeIndices.size() * sizeof(uint16_t), edgeIndices.data(), GL_DYNAMIC_DRAW);

        glDrawElements(GL_LINES, (GLsizei)edgeIndices.size(), GL_UNSIGNED_SHORT, (void*)0);
        glDeleteBuffers(1, &edgeEbo);
    }

    if (ebo) glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Viewport3D::renderGizmo(const Vec3& pos, const Mat4& vp) {
    glUseProgram(m_progFlat);
    glDisable(GL_DEPTH_TEST);

    Mat4 model = Mat4::translation(pos);
    Mat4 mvp = vp * model;
    glUniformMatrix4fv(m_uFlatMVP, 1, GL_FALSE, mvp.m);

    float axisLen = 1.2f;
    float arrowData[18] = {
        0.0f, 0.0f, 0.0f,   axisLen, 0.0f, 0.0f, // X (Red)
        0.0f, 0.0f, 0.0f,   0.0f, axisLen, 0.0f, // Y (Green)
        0.0f, 0.0f, 0.0f,   0.0f, 0.0f, axisLen  // Z (Blue)
    };

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(arrowData), arrowData, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // X Axis: Red
    glUniform4f(m_uFlatColor, 0.95f, 0.20f, 0.20f, 1.0f);
    glDrawArrays(GL_LINES, 0, 2);

    // Y Axis: Green
    glUniform4f(m_uFlatColor, 0.20f, 0.85f, 0.20f, 1.0f);
    glDrawArrays(GL_LINES, 2, 2);

    // Z Axis: Blue
    glUniform4f(m_uFlatColor, 0.25f, 0.50f, 0.95f, 1.0f);
    glDrawArrays(GL_LINES, 4, 2);

    glEnable(GL_DEPTH_TEST);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Viewport3D::renderCursor3D(const Vec3& pos, const Mat4& vp) {
    glUseProgram(m_progFlat);
    Mat4 model = Mat4::translation(pos);
    Mat4 mvp = vp * model;
    glUniformMatrix4fv(m_uFlatMVP, 1, GL_FALSE, mvp.m);

    float s = 0.35f;
    float crossData[18] = {
        -s, 0, 0,   s, 0, 0,
        0, -s, 0,   0, s, 0,
        0, 0, -s,   0, 0, s
    };

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crossData), crossData, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glUniform4f(m_uFlatColor, 0.95f, 0.25f, 0.25f, 1.0f);
    glDrawArrays(GL_LINES, 0, 6);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Viewport3D::getRayFromScreen(float mouseX, float mouseY, Vec3& outOrigin, Vec3& outDir) {
    float aspect = (float)m_width / (float)(m_height > 0 ? m_height : 1);
    float fov = 45.0f * (3.14159265f / 180.0f);
    float ndcX = (2.0f * mouseX) / (float)m_width - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / (float)m_height;

    outOrigin = getCameraEye();
    Vec3 f = (m_target - outOrigin).normalized();
    Vec3 r = Vec3::cross(f, Vec3(0, 1, 0)).normalized();
    Vec3 u = Vec3::cross(r, f);

    float halfH = std::tan(fov * 0.5f);
    float halfW = halfH * aspect;

    outDir = (f + r * (ndcX * halfW) + u * (ndcY * halfH)).normalized();
}

int Viewport3D::pickObject(const Scene& scene, float mouseX, float mouseY) {
    Vec3 eye, rayDir;
    getRayFromScreen(mouseX, mouseY, eye, rayDir);

    int bestIdx = -1;
    float bestDist = 1e9f;

    for (size_t i = 0; i < scene.objects.size(); ++i) {
        const auto& obj = scene.objects[i];
        if (!obj.visible || !obj.selectable) continue;

        Vec3 oc = obj.location - eye;
        float tca = Vec3::dot(oc, rayDir);
        if (tca < 0) continue;

        float d2 = Vec3::dot(oc, oc) - tca * tca;
        float radius2 = 1.8f * 1.8f * (obj.scale.x * obj.scale.x);
        if (d2 <= radius2) {
            float dist = tca;
            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = (int)i;
            }
        }
    }

    return bestIdx;
}

bool Viewport3D::raycastSurface(const SceneObject& obj, const Vec3& rayOrigin, const Vec3& rayDir, Vec3& outHitPos, Vec3& outHitNormal) {
    if (obj.mesh.vertices.empty() || obj.mesh.faces.empty()) return false;

    Mat4 model = obj.getTransformMatrix();
    float bestT = 1e9f;
    bool found = false;

    for (const auto& f : obj.mesh.faces) {
        if (f.v0 >= obj.mesh.vertices.size() || f.v1 >= obj.mesh.vertices.size() || f.v2 >= obj.mesh.vertices.size()) continue;

        // Transform vertices to world space
        auto transformPt = [&](const Vec3& p) -> Vec3 {
            return Vec3(
                model.m[0]*p.x + model.m[4]*p.y + model.m[8]*p.z  + model.m[12],
                model.m[1]*p.x + model.m[5]*p.y + model.m[9]*p.z  + model.m[13],
                model.m[2]*p.x + model.m[6]*p.y + model.m[10]*p.z + model.m[14]
            );
        };

        Vec3 p0 = transformPt(obj.mesh.vertices[f.v0].pos);
        Vec3 p1 = transformPt(obj.mesh.vertices[f.v1].pos);
        Vec3 p2 = transformPt(obj.mesh.vertices[f.v2].pos);

        Vec3 e1 = p1 - p0;
        Vec3 e2 = p2 - p0;
        Vec3 h = Vec3::cross(rayDir, e2);
        float a = Vec3::dot(e1, h);
        if (std::abs(a) < 1e-6f) continue;

        float f_inv = 1.0f / a;
        Vec3 s = rayOrigin - p0;
        float u = f_inv * Vec3::dot(s, h);
        if (u < 0.0f || u > 1.0f) continue;

        Vec3 q = Vec3::cross(s, e1);
        float v = f_inv * Vec3::dot(rayDir, q);
        if (v < 0.0f || u + v > 1.0f) continue;

        float t = f_inv * Vec3::dot(e2, q);
        if (t > 1e-4f && t < bestT) {
            bestT = t;
            outHitPos = rayOrigin + rayDir * t;
            outHitNormal = Vec3::cross(e1, e2).normalized();
            found = true;
        }
    }

    return found;
}

void Viewport3D::render(Scene& scene) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);

    // Blender 3D Viewport dark slate canvas (#303030 / #343434)
    float bgR = 0.20f, bgG = 0.20f, bgB = 0.21f;
    glClearColor(bgR, bgG, bgB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    float aspect = (float)m_width / (float)(m_height > 0 ? m_height : 1);
    float fov = 45.0f * (3.14159265f / 180.0f);
    Mat4 proj = Mat4::perspective(fov, aspect, 0.1f, 100.0f);
    Mat4 view = Mat4::lookAt(getCameraEye(), m_target, Vec3(0, 1, 0));
    Mat4 vp = proj * view;

    // 1. Render Ground Grid & Coordinate Axes
    renderGrid(vp, scene.showFloorGrid, scene.showAxes);

    // 2. Render Scene Objects
    for (size_t i = 0; i < scene.objects.size(); ++i) {
        bool isSelected = ((int)i == scene.selectedIndex);
        renderObject(scene.objects[i], vp, view, isSelected, scene);
    }

    // 3. Render 3D Cursor
    if (scene.show3DCursor) {
        renderCursor3D(scene.cursor3D, vp);
    }

    // 4. Render Transform Gizmo at selected object location
    SceneObject* sel = scene.getSelectedObject();
    if (sel && sel->visible) {
        renderGizmo(sel->location, vp);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
