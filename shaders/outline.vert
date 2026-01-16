// =============================================================================
// OUTLINE VERTEX SHADER - For object outline rendering
// =============================================================================
#version 450

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
    mat4 worldMatrix;
    uvec2 vertexBuffer;
    float outlineScale;
    float _pad0;
    vec4 baseColor;
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
