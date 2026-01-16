#version 450

// =============================================================================
// EMISSIVE VERTEX SHADER - For point light visualization spheres
// =============================================================================

layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec4 inColor;

// Push constants for per-object data
layout(push_constant) uniform PushConstantData {
    mat4 worldMatrix;
    vec2 vertexBufferAddress;   // Compatibility padding
    float outlineScale;
    float _pad0;
    vec4 baseColor;
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
