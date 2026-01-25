#version 450

// =============================================================================
// EMISSIVE VERTEX SHADER - For point light visualization spheres
// =============================================================================

layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec4 inColor;

// Push constants - must match C++ GPUDrawPushConstants (112 bytes)
layout(push_constant) uniform PushConstantData {
    mat4 worldMatrix;          // 64 bytes, offset 0-63
    uvec2 vertexBuffer;        // 8 bytes, offset 64-71 (VkDeviceAddress = uint64)
    float outlineScale;        // 4 bytes, offset 72-75
    float padding[5];          // 20 bytes, offset 76-95 (for vec4 16-byte alignment)
    vec4 baseColor;            // 16 bytes, offset 96-111
} push;

// Scene data - MUST match GPUSceneData in vk_types.h
// We only need viewproj for this shader, but buffer layout must match
#define MAX_POINT_LIGHTS 64

struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;
};

layout(set = 0, binding = 0) uniform SceneDataBuffer {
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

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec4 fragBaseColor;

void main() {
    vec4 worldPos = push.worldMatrix * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragBaseColor = push.baseColor;
    gl_Position = sceneData.viewproj * worldPos;
}
