#version 450

// =============================================================================
// PRIMITIVE VERTEX SHADER - With Face Color Support
// =============================================================================
// Supports per-face coloring based on normal direction
// Full lighting support for professional rendering
// =============================================================================

// Vertex inputs (matches Vertex struct in vk_types.h)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inUV_X;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in float inUV_Y;
layout(location = 4) in vec4 inColor;

// Outputs to fragment shader
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec2 fragUV;
layout(location = 4) flat out int fragFaceIndex;

// Push constants for primitive rendering - MUST match C++ struct
layout(push_constant) uniform PushConstants {
    mat4 worldMatrix;       // 64 bytes (offset 0)
    vec4 mainColor;         // 16 bytes (offset 64) - RGBA base color
    vec4 faceColors[6];     // 96 bytes (offset 80) - Per-face colors
    vec4 pbrParams;         // 16 bytes (offset 176) - x=metallic, y=roughness, z=ao, w=unused
    vec4 emission;          // 16 bytes (offset 192) - xyz=emission color, w=emission strength
    int useFaceColors;      // 4 bytes (offset 208)
    int padding[3];         // 12 bytes (offset 212)
} push;

// Scene data - MUST match GPUSceneData in vk_types.h
#define MAX_POINT_LIGHTS 64

struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;
};

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
    vec4 cameraPosition;
    PointLight pointLights[MAX_POINT_LIGHTS];
    int pointLightCount;
    float _pad0;
    float _pad1;
    float _pad2;
} sceneData;

// Determine face index from normal direction
// 0 = Front (+Z), 1 = Back (-Z), 2 = Right (+X), 3 = Left (-X), 4 = Top (+Y), 5 = Bottom (-Y)
int getFaceIndexFromNormal(vec3 normal) {
    vec3 absNormal = abs(normal);

    if (absNormal.z >= absNormal.x && absNormal.z >= absNormal.y) {
        return normal.z > 0.0 ? 0 : 1;  // Front or Back
    } else if (absNormal.x >= absNormal.y) {
        return normal.x > 0.0 ? 2 : 3;  // Right or Left
    } else {
        return normal.y > 0.0 ? 4 : 5;  // Top or Bottom
    }
}

void main() {
    // Transform position to world space
    vec4 worldPos = push.worldMatrix * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    // Transform normal to world space (use inverse transpose for non-uniform scale)
    mat3 normalMatrix = mat3(push.worldMatrix);
    fragNormal = normalize(normalMatrix * inNormal);

    // Determine face index for per-face coloring
    fragFaceIndex = getFaceIndexFromNormal(inNormal);

    // Pass through color and UV
    fragColor = inColor * push.mainColor;
    fragUV = vec2(inUV_X, inUV_Y);

    // Final clip space position
    gl_Position = sceneData.viewproj * worldPos;
}
