// =============================================================================
// OUTLINE VERTEX SHADER - For object outline rendering
// =============================================================================
#version 450

layout(location = 0) in vec3 inPosition;

// Push constants - must match C++ GPUDrawPushConstants (112 bytes)
layout(push_constant) uniform PushConstants {
    mat4 worldMatrix;          // 64 bytes, offset 0-63
    uvec2 vertexBuffer;        // 8 bytes, offset 64-71 (VkDeviceAddress = uint64)
    float outlineScale;        // 4 bytes, offset 72-75
    float padding[5];          // 20 bytes, offset 76-95 (for vec4 16-byte alignment)
    vec4 baseColor;            // 16 bytes, offset 96-111
} pushConstants;

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

void main()
{
    vec4 worldPos = pushConstants.worldMatrix * vec4(inPosition, 1.0);
    worldPos.xyz += normalize(worldPos.xyz) * pushConstants.outlineScale;
    gl_Position = sceneData.proj * sceneData.view * worldPos;
}
