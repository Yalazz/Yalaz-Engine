#version 450

// =============================================================================
// SHADOW VERTEX SHADER - Depth-only rendering for shadow maps
// =============================================================================

layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inUV_X;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in float inUV_Y;
layout(location = 4) in vec4 inColor;

// Push constant: world matrix + cascade index
layout(push_constant) uniform PushConstants {
    mat4 worldMatrix;      // 64 bytes
    int cascadeIndex;      // 4 bytes - which cascade we're rendering
} push;

// Scene data for light-space matrix (cascade 0 for now)
#define MAX_POINT_LIGHTS 64
#define SHADOW_CASCADE_COUNT 4

struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;
};

layout(std140, set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
    vec4 cameraPosition;
    PointLight pointLights[MAX_POINT_LIGHTS];
    int pointLightCount;
    float shadowBias;
    float shadowNormalBias;
    int shadowsEnabled;
    mat4 shadowMatrices[SHADOW_CASCADE_COUNT];
    vec4 cascadeSplits;
} sceneData;

void main() {
    // Transform vertex to world space, then to light space for the current cascade
    vec4 worldPos = push.worldMatrix * vec4(inPosition, 1.0);
    gl_Position = sceneData.shadowMatrices[push.cascadeIndex] * worldPos;
}
