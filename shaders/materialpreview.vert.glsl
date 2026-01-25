// =============================================================================
// MATERIAL PREVIEW VERTEX SHADER
// =============================================================================
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inUV_x;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in float inUV_y;
layout(location = 4) in vec4 inColor;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec4 fragColor;

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

layout(push_constant) uniform PushConstants {
    mat4 model;
} pushData;

void main() {
    mat4 modelView = sceneData.view * pushData.model;
    gl_Position = sceneData.viewproj * pushData.model * vec4(inPosition, 1.0);
    fragNormal = mat3(modelView) * inNormal;
    fragColor = inColor;
}
