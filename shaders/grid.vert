#version 450

// =============================================================================
// PROFESSIONAL DYNAMIC GRID VERTEX SHADER
// =============================================================================

layout(location = 0) in vec3 inPosition;

// Push constants - must match C++ GridPushConstants and grid.frag
layout(push_constant) uniform PushConstants {
    mat4 worldMatrix;
    vec4 gridParams;
    vec4 gridParams2;
    vec4 gridParams3;
    vec4 minorColor;
    vec4 majorColor;
    vec4 xAxisColor;
    vec4 zAxisColor;
} pc;

// Scene data
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

// Outputs
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 nearPoint;
layout(location = 2) out vec3 farPoint;
layout(location = 3) out float fragCameraHeight;
layout(location = 4) out vec2 fragCameraPosXZ;  // Camera XZ position for distance fade

void main() {
    vec4 worldPosition = pc.worldMatrix * vec4(inPosition, 1.0);
    fragWorldPos = worldPosition.xyz;
    fragCameraHeight = sceneData.cameraPosition.y;
    fragCameraPosXZ = sceneData.cameraPosition.xz;  // Pass camera XZ to fragment
    nearPoint = vec3(0.0);
    farPoint = vec3(0.0);
    gl_Position = sceneData.proj * sceneData.view * worldPosition;
}
