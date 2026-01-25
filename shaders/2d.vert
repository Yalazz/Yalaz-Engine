#version 450

// =============================================================================
// 2D/3D PRIMITIVE VERTEX SHADER - With Lighting Support
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

// Push constants - must match C++ GPUDrawPushConstants (112 bytes)
layout(push_constant) uniform PushConstants {
    mat4 worldMatrix;          // 64 bytes, offset 0-63
    uvec2 vertexBuffer;        // 8 bytes, offset 64-71 (VkDeviceAddress = uint64)
    float outlineScale;        // 4 bytes, offset 72-75
    float padding[5];          // 20 bytes, offset 76-95 (for vec4 16-byte alignment)
    vec4 baseColor;            // 16 bytes, offset 96-111
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

void main() {
    // Transform position to world space
    vec4 worldPos = push.worldMatrix * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    // Transform normal to world space (use inverse transpose for non-uniform scale)
    mat3 normalMatrix = mat3(push.worldMatrix);
    fragNormal = normalize(normalMatrix * inNormal);

    // Pass through color and UV
    fragColor = inColor * push.baseColor;
    fragUV = vec2(inUV_X, inUV_Y);

    // Final clip space position
    gl_Position = sceneData.viewproj * worldPos;
}
