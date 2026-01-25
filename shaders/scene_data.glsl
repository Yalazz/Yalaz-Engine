// =============================================================================
// SCENE DATA - Legacy shared include (set 1 version)
// =============================================================================
// NOTE: This uses set 1, binding 0 for legacy compatibility.
// For new shaders, use input_structures.glsl which uses set 0, binding 0
// =============================================================================

#ifndef SCENE_DATA_LEGACY_GLSL
#define SCENE_DATA_LEGACY_GLSL

#define MAX_POINT_LIGHTS 64

struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;
};

layout(set = 1, binding = 0) uniform SceneData {
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
