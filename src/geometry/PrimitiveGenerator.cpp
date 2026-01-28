#include "PrimitiveGenerator.h"
#include "vk_types.h"
#include <fmt/core.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Yalaz::Geometry {

void PrimitiveGenerator::OnInit() {
    fmt::print("[PrimitiveGenerator] Initialized\n");
}

void PrimitiveGenerator::OnShutdown() {
    m_UploadFunc = nullptr;
    fmt::print("[PrimitiveGenerator] Shutdown\n");
}

GPUMeshBuffers PrimitiveGenerator::UploadMesh(MeshData& data) {
    if (!m_UploadFunc) {
        fmt::print("[PrimitiveGenerator] Error: Upload function not set!\n");
        return {};
    }
    return m_UploadFunc(data.indices, data.vertices);
}

// =========================================================================
// CPU-side mesh data generation
// =========================================================================

MeshData PrimitiveGenerator::GenerateTriangleData() {
    MeshData data;
    glm::vec3 normal(0.0f, 0.0f, 1.0f);
    data.vertices = {
        { glm::vec3(0, 1, 0), 0.5f, normal, 1.0f, glm::vec4(1) },
        { glm::vec3(-1, -1, 0), 0.0f, normal, 0.0f, glm::vec4(1) },
        { glm::vec3(1, -1, 0), 1.0f, normal, 0.0f, glm::vec4(1) },
    };
    data.indices = { 0, 1, 2 };
    return data;
}

MeshData PrimitiveGenerator::GeneratePlaneData() {
    MeshData data;
    data.vertices = {
        { { -1, 0, -1 }, 0.0f, { 0, 1, 0 }, 0.0f, glm::vec4(1) },
        { {  1, 0, -1 }, 1.0f, { 0, 1, 0 }, 0.0f, glm::vec4(1) },
        { {  1, 0,  1 }, 1.0f, { 0, 1, 0 }, 1.0f, glm::vec4(1) },
        { { -1, 0,  1 }, 0.0f, { 0, 1, 0 }, 1.0f, glm::vec4(1) },
    };
    data.indices = { 0, 1, 2, 2, 3, 0 };
    return data;
}

MeshData PrimitiveGenerator::GenerateCubeData() {
    MeshData data;
    auto& vertices = data.vertices;
    auto& indices = data.indices;

    // Unit cube from -0.5 to +0.5, with per-face normals

    // Front face (+Z)
    vertices.push_back({ { -0.5f, -0.5f,  0.5f }, 0.0f, { 0, 0, 1 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ {  0.5f, -0.5f,  0.5f }, 1.0f, { 0, 0, 1 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ {  0.5f,  0.5f,  0.5f }, 1.0f, { 0, 0, 1 }, 1.0f, glm::vec4(1) });
    vertices.push_back({ { -0.5f,  0.5f,  0.5f }, 0.0f, { 0, 0, 1 }, 1.0f, glm::vec4(1) });

    // Back face (-Z)
    vertices.push_back({ {  0.5f, -0.5f, -0.5f }, 0.0f, { 0, 0, -1 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ { -0.5f, -0.5f, -0.5f }, 1.0f, { 0, 0, -1 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ { -0.5f,  0.5f, -0.5f }, 1.0f, { 0, 0, -1 }, 1.0f, glm::vec4(1) });
    vertices.push_back({ {  0.5f,  0.5f, -0.5f }, 0.0f, { 0, 0, -1 }, 1.0f, glm::vec4(1) });

    // Right face (+X)
    vertices.push_back({ {  0.5f, -0.5f,  0.5f }, 0.0f, { 1, 0, 0 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ {  0.5f, -0.5f, -0.5f }, 1.0f, { 1, 0, 0 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ {  0.5f,  0.5f, -0.5f }, 1.0f, { 1, 0, 0 }, 1.0f, glm::vec4(1) });
    vertices.push_back({ {  0.5f,  0.5f,  0.5f }, 0.0f, { 1, 0, 0 }, 1.0f, glm::vec4(1) });

    // Left face (-X)
    vertices.push_back({ { -0.5f, -0.5f, -0.5f }, 0.0f, { -1, 0, 0 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ { -0.5f, -0.5f,  0.5f }, 1.0f, { -1, 0, 0 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ { -0.5f,  0.5f,  0.5f }, 1.0f, { -1, 0, 0 }, 1.0f, glm::vec4(1) });
    vertices.push_back({ { -0.5f,  0.5f, -0.5f }, 0.0f, { -1, 0, 0 }, 1.0f, glm::vec4(1) });

    // Top face (+Y)
    vertices.push_back({ { -0.5f,  0.5f,  0.5f }, 0.0f, { 0, 1, 0 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ {  0.5f,  0.5f,  0.5f }, 1.0f, { 0, 1, 0 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ {  0.5f,  0.5f, -0.5f }, 1.0f, { 0, 1, 0 }, 1.0f, glm::vec4(1) });
    vertices.push_back({ { -0.5f,  0.5f, -0.5f }, 0.0f, { 0, 1, 0 }, 1.0f, glm::vec4(1) });

    // Bottom face (-Y)
    vertices.push_back({ { -0.5f, -0.5f, -0.5f }, 0.0f, { 0, -1, 0 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ {  0.5f, -0.5f, -0.5f }, 1.0f, { 0, -1, 0 }, 0.0f, glm::vec4(1) });
    vertices.push_back({ {  0.5f, -0.5f,  0.5f }, 1.0f, { 0, -1, 0 }, 1.0f, glm::vec4(1) });
    vertices.push_back({ { -0.5f, -0.5f,  0.5f }, 0.0f, { 0, -1, 0 }, 1.0f, glm::vec4(1) });

    // Indices (6 faces x 2 triangles x 3 vertices = 36)
    for (int face = 0; face < 6; face++) {
        uint32_t base = face * 4;
        indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base + 2); indices.push_back(base + 3); indices.push_back(base + 0);
    }

    return data;
}

MeshData PrimitiveGenerator::GenerateSphereData(int resolution, int rings) {
    MeshData data;
    auto& vertices = data.vertices;
    auto& indices = data.indices;

    for (int y = 0; y <= rings; y++) {
        for (int x = 0; x <= resolution; x++) {
            float xSeg = (float)x / resolution;
            float ySeg = (float)y / rings;
            float xPos = std::cos(xSeg * 2.0f * (float)M_PI) * std::sin(ySeg * (float)M_PI);
            float yPos = std::cos(ySeg * (float)M_PI);
            float zPos = std::sin(xSeg * 2.0f * (float)M_PI) * std::sin(ySeg * (float)M_PI);

            Vertex v;
            v.position = glm::vec3(xPos, yPos, zPos) * 0.5f;
            v.normal = glm::vec3(xPos, yPos, zPos);
            v.uv_x = xSeg;
            v.uv_y = ySeg;
            v.color = glm::vec4(1);
            vertices.push_back(v);
        }
    }

    for (int y = 0; y < rings; y++) {
        for (int x = 0; x < resolution; x++) {
            uint32_t i0 = y * (resolution + 1) + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + (resolution + 1);
            uint32_t i3 = i2 + 1;

            indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
            indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
        }
    }

    return data;
}

MeshData PrimitiveGenerator::GenerateCylinderData(int segments) {
    MeshData data;
    auto& vertices = data.vertices;
    auto& indices = data.indices;

    float halfHeight = 0.5f;
    float radius = 0.5f;

    // Side vertices
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * (float)M_PI;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0, z));

        // Bottom vertex
        vertices.push_back({ { x, -halfHeight, z }, (float)i / segments, normal, 0.0f, glm::vec4(1) });
        // Top vertex
        vertices.push_back({ { x, halfHeight, z }, (float)i / segments, normal, 1.0f, glm::vec4(1) });
    }

    // Side indices
    for (int i = 0; i < segments; i++) {
        uint32_t base = i * 2;
        indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base + 2); indices.push_back(base + 1); indices.push_back(base + 3);
    }

    // Top cap center
    uint32_t topCenter = (uint32_t)vertices.size();
    vertices.push_back({ { 0, halfHeight, 0 }, 0.5f, { 0, 1, 0 }, 0.5f, glm::vec4(1) });

    // Top cap vertices
    uint32_t topStart = (uint32_t)vertices.size();
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * (float)M_PI;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        vertices.push_back({ { x, halfHeight, z }, x * 0.5f + 0.5f, { 0, 1, 0 }, z * 0.5f + 0.5f, glm::vec4(1) });
    }

    // Top cap indices
    for (int i = 0; i < segments; i++) {
        indices.push_back(topCenter);
        indices.push_back(topStart + i + 1);
        indices.push_back(topStart + i);
    }

    // Bottom cap center
    uint32_t botCenter = (uint32_t)vertices.size();
    vertices.push_back({ { 0, -halfHeight, 0 }, 0.5f, { 0, -1, 0 }, 0.5f, glm::vec4(1) });

    // Bottom cap vertices
    uint32_t botStart = (uint32_t)vertices.size();
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * (float)M_PI;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        vertices.push_back({ { x, -halfHeight, z }, x * 0.5f + 0.5f, { 0, -1, 0 }, z * 0.5f + 0.5f, glm::vec4(1) });
    }

    // Bottom cap indices
    for (int i = 0; i < segments; i++) {
        indices.push_back(botCenter);
        indices.push_back(botStart + i);
        indices.push_back(botStart + i + 1);
    }

    return data;
}

MeshData PrimitiveGenerator::GenerateConeData(int segments) {
    MeshData data;
    auto& vertices = data.vertices;
    auto& indices = data.indices;

    float halfHeight = 0.5f;
    float radius = 0.5f;

    // Apex
    uint32_t apex = (uint32_t)vertices.size();
    vertices.push_back({ { 0, halfHeight, 0 }, 0.5f, { 0, 1, 0 }, 0.5f, glm::vec4(1) });

    // Base circle
    uint32_t baseStart = (uint32_t)vertices.size();
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * (float)M_PI;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        glm::vec3 normal = glm::normalize(glm::vec3(x, radius, z));
        vertices.push_back({ { x, -halfHeight, z }, (float)i / segments, normal, 0.0f, glm::vec4(1) });
    }

    // Side indices
    for (int i = 0; i < segments; i++) {
        indices.push_back(apex);
        indices.push_back(baseStart + i);
        indices.push_back(baseStart + i + 1);
    }

    // Base cap center
    uint32_t baseCenter = (uint32_t)vertices.size();
    vertices.push_back({ { 0, -halfHeight, 0 }, 0.5f, { 0, -1, 0 }, 0.5f, glm::vec4(1) });

    // Base cap vertices
    uint32_t capStart = (uint32_t)vertices.size();
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * (float)M_PI;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        vertices.push_back({ { x, -halfHeight, z }, x * 0.5f + 0.5f, { 0, -1, 0 }, z * 0.5f + 0.5f, glm::vec4(1) });
    }

    // Base cap indices
    for (int i = 0; i < segments; i++) {
        indices.push_back(baseCenter);
        indices.push_back(capStart + i);
        indices.push_back(capStart + i + 1);
    }

    return data;
}

MeshData PrimitiveGenerator::GenerateCapsuleData(int segments, int rings) {
    MeshData data;
    auto& vertices = data.vertices;
    auto& indices = data.indices;

    float radius = 0.25f;
    float halfHeight = 0.25f;

    // Top hemisphere
    for (int y = 0; y <= rings; y++) {
        for (int x = 0; x <= segments; x++) {
            float xSeg = (float)x / segments;
            float ySeg = (float)y / rings * 0.5f;
            float xPos = std::cos(xSeg * 2.0f * (float)M_PI) * std::sin(ySeg * (float)M_PI);
            float yPos = std::cos(ySeg * (float)M_PI);
            float zPos = std::sin(xSeg * 2.0f * (float)M_PI) * std::sin(ySeg * (float)M_PI);

            Vertex v;
            v.position = glm::vec3(xPos * radius, yPos * radius + halfHeight, zPos * radius);
            v.normal = glm::vec3(xPos, yPos, zPos);
            v.uv_x = xSeg;
            v.uv_y = ySeg;
            v.color = glm::vec4(1);
            vertices.push_back(v);
        }
    }

    // Cylinder body
    uint32_t cylStart = (uint32_t)vertices.size();
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * (float)M_PI;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0, z));

        vertices.push_back({ { x, halfHeight, z }, (float)i / segments, normal, 0.25f, glm::vec4(1) });
        vertices.push_back({ { x, -halfHeight, z }, (float)i / segments, normal, 0.75f, glm::vec4(1) });
    }

    // Bottom hemisphere
    uint32_t botStart = (uint32_t)vertices.size();
    for (int y = 0; y <= rings; y++) {
        for (int x = 0; x <= segments; x++) {
            float xSeg = (float)x / segments;
            float ySeg = 0.5f + (float)y / rings * 0.5f;
            float xPos = std::cos(xSeg * 2.0f * (float)M_PI) * std::sin(ySeg * (float)M_PI);
            float yPos = std::cos(ySeg * (float)M_PI);
            float zPos = std::sin(xSeg * 2.0f * (float)M_PI) * std::sin(ySeg * (float)M_PI);

            Vertex v;
            v.position = glm::vec3(xPos * radius, yPos * radius - halfHeight, zPos * radius);
            v.normal = glm::vec3(xPos, yPos, zPos);
            v.uv_x = xSeg;
            v.uv_y = 0.75f + ySeg * 0.25f;
            v.color = glm::vec4(1);
            vertices.push_back(v);
        }
    }

    // Top hemisphere indices
    for (int y = 0; y < rings; y++) {
        for (int x = 0; x < segments; x++) {
            uint32_t i0 = y * (segments + 1) + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + (segments + 1);
            uint32_t i3 = i2 + 1;
            indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
            indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
        }
    }

    // Cylinder body indices
    for (int i = 0; i < segments; i++) {
        uint32_t base = cylStart + i * 2;
        indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base + 2); indices.push_back(base + 1); indices.push_back(base + 3);
    }

    // Bottom hemisphere indices
    for (int y = 0; y < rings; y++) {
        for (int x = 0; x < segments; x++) {
            uint32_t i0 = botStart + y * (segments + 1) + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + (segments + 1);
            uint32_t i3 = i2 + 1;
            indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
            indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
        }
    }

    return data;
}

MeshData PrimitiveGenerator::GenerateTorusData(int segments, int rings) {
    MeshData data;
    auto& vertices = data.vertices;
    auto& indices = data.indices;

    float majorRadius = 0.35f;
    float minorRadius = 0.15f;

    for (int i = 0; i <= rings; i++) {
        float u = (float)i / rings * 2.0f * (float)M_PI;
        float cu = std::cos(u);
        float su = std::sin(u);

        for (int j = 0; j <= segments; j++) {
            float v = (float)j / segments * 2.0f * (float)M_PI;
            float cv = std::cos(v);
            float sv = std::sin(v);

            float x = (majorRadius + minorRadius * cv) * cu;
            float y = minorRadius * sv;
            float z = (majorRadius + minorRadius * cv) * su;

            glm::vec3 normal = glm::normalize(glm::vec3(cv * cu, sv, cv * su));

            Vertex vert;
            vert.position = glm::vec3(x, y, z);
            vert.normal = normal;
            vert.uv_x = (float)i / rings;
            vert.uv_y = (float)j / segments;
            vert.color = glm::vec4(1);
            vertices.push_back(vert);
        }
    }

    for (int i = 0; i < rings; i++) {
        for (int j = 0; j < segments; j++) {
            uint32_t i0 = i * (segments + 1) + j;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + (segments + 1);
            uint32_t i3 = i2 + 1;

            indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
            indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
        }
    }

    return data;
}

MeshData PrimitiveGenerator::GenerateGridData(int numX, int numZ, float spacing) {
    MeshData data;
    auto& vertices = data.vertices;
    auto& indices = data.indices;

    float halfX = (numX * spacing) / 2.0f;
    float halfZ = (numZ * spacing) / 2.0f;

    // Generate grid lines in X direction
    for (int i = 0; i <= numX; i++) {
        float x = -halfX + i * spacing;
        vertices.push_back({ { x, 0, -halfZ }, 0, { 0, 1, 0 }, 0, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) });
        vertices.push_back({ { x, 0, halfZ }, 1, { 0, 1, 0 }, 1, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) });
    }

    // Generate grid lines in Z direction
    for (int i = 0; i <= numZ; i++) {
        float z = -halfZ + i * spacing;
        vertices.push_back({ { -halfX, 0, z }, 0, { 0, 1, 0 }, 0, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) });
        vertices.push_back({ { halfX, 0, z }, 1, { 0, 1, 0 }, 1, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) });
    }

    // Generate indices for line rendering
    for (uint32_t i = 0; i < vertices.size(); i += 2) {
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    return data;
}

MeshData PrimitiveGenerator::GenerateData(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Triangle: return GenerateTriangleData();
        case PrimitiveType::Plane:    return GeneratePlaneData();
        case PrimitiveType::Cube:     return GenerateCubeData();
        case PrimitiveType::Sphere:   return GenerateSphereData();
        case PrimitiveType::Cylinder: return GenerateCylinderData();
        case PrimitiveType::Cone:     return GenerateConeData();
        case PrimitiveType::Capsule:  return GenerateCapsuleData();
        case PrimitiveType::Torus:    return GenerateTorusData();
        default:                      return GenerateCubeData();
    }
}

// =========================================================================
// GPU mesh generation (delegates to upload function)
// =========================================================================

GPUMeshBuffers PrimitiveGenerator::GenerateTriangle() {
    auto data = GenerateTriangleData();
    return UploadMesh(data);
}

GPUMeshBuffers PrimitiveGenerator::GeneratePlane() {
    auto data = GeneratePlaneData();
    return UploadMesh(data);
}

GPUMeshBuffers PrimitiveGenerator::GenerateCube() {
    auto data = GenerateCubeData();
    return UploadMesh(data);
}

GPUMeshBuffers PrimitiveGenerator::GenerateSphere(int resolution, int rings) {
    auto data = GenerateSphereData(resolution, rings);
    return UploadMesh(data);
}

GPUMeshBuffers PrimitiveGenerator::GenerateCylinder(int segments) {
    auto data = GenerateCylinderData(segments);
    return UploadMesh(data);
}

GPUMeshBuffers PrimitiveGenerator::GenerateCone(int segments) {
    auto data = GenerateConeData(segments);
    return UploadMesh(data);
}

GPUMeshBuffers PrimitiveGenerator::GenerateCapsule(int segments, int rings) {
    auto data = GenerateCapsuleData(segments, rings);
    return UploadMesh(data);
}

GPUMeshBuffers PrimitiveGenerator::GenerateTorus(int segments, int rings) {
    auto data = GenerateTorusData(segments, rings);
    return UploadMesh(data);
}

GPUMeshBuffers PrimitiveGenerator::Generate(PrimitiveType type) {
    auto data = GenerateData(type);
    return UploadMesh(data);
}

} // namespace Yalaz::Geometry
