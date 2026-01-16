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

// Push constants
layout(push_constant) uniform PushConstants {
    mat4 worldMatrix;
    uint vertexBufferAddress;  // 8 bytes (VkDeviceAddress)
    float outlineScale;
    float padding[3];
    vec4 baseColor;
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
