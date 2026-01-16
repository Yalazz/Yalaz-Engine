#version 450

// =============================================================================
// CHUNKED GRID VERTEX SHADER - High Performance
// =============================================================================

layout(location = 0) in vec3 inPosition;

// Per-instance data (chunk position and scale)
layout(location = 1) in vec4 instancePosScale;  // xyz = chunk center, w = chunk size
layout(location = 2) in vec4 instanceLodParams; // x = lod, y = opacity

// Scene data
layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
    vec4 cameraPosition;
} sceneData;

// Push constants for grid settings
layout(push_constant) uniform PushConstants {
    vec4 gridParams;   // x=cellSize, y=fadeDistance, z=lineWidth, w=opacity
    vec4 gridParams2;  // x=majorMultiplier, yzw=reserved
    vec4 minorColor;
    vec4 majorColor;
    vec4 axisColors;   // xy = X axis RG, zw = Z axis RG (packed)
} pc;

// Outputs
layout(location = 0) out vec2 fragGridPos;
layout(location = 1) out float fragLodLevel;
layout(location = 2) out float fragOpacity;
layout(location = 3) out float fragDistToCamera;

void main() {
    // Scale vertex by chunk size and offset by chunk position
    vec3 worldPos = inPosition * instancePosScale.w + vec3(instancePosScale.x, 0.0, instancePosScale.z);

    // Output grid position (XZ plane)
    fragGridPos = worldPos.xz;

    // Pass through LOD and opacity
    fragLodLevel = instanceLodParams.x;
    fragOpacity = instanceLodParams.y * pc.gridParams.w;

    // Distance to camera for additional fade
    fragDistToCamera = length(worldPos.xz - sceneData.cameraPosition.xz);

    gl_Position = sceneData.viewproj * vec4(worldPos, 1.0);
}
