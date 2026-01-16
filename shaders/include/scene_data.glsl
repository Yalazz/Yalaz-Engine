// =============================================================================
// SCENE DATA - Shared GPU uniform buffer definition
// =============================================================================
// CRITICAL: This file MUST match GPUSceneData in vk_types.h
// =============================================================================

#ifndef SCENE_DATA_GLSL
#define SCENE_DATA_GLSL

#define MAX_POINT_LIGHTS 64

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
    float _pad0;
    float _pad1;
    float _pad2;
} sceneData;

#endif
