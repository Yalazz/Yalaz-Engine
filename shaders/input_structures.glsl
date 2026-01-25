// =============================================================================
// INPUT STRUCTURES - GPU Shader Uniforms (std140 layout)
// =============================================================================
// CRITICAL: This file MUST match the C++ GPUSceneData struct in vk_types.h
// Any mismatch will cause lighting to fail silently!
// =============================================================================

#ifndef INPUT_STRUCTURES_GLSL
#define INPUT_STRUCTURES_GLSL

// Must match C++ MAX_POINT_LIGHTS in vk_types.h
#define MAX_POINT_LIGHTS 64

// Point light structure - 32 bytes (matches C++ PointLight)
// Layout: [position.xyz, radius] [color.xyz, intensity]
struct PointLight {
    vec3 position;      // offset 0,  size 12
    float radius;       // offset 12, size 4
    vec3 color;         // offset 16, size 12
    float intensity;    // offset 28, size 4
};  // Total: 32 bytes

// Scene data uniform buffer (set 0, binding 0)
// Total size: 2320 bytes (must match C++ GPUSceneData)
// Using std140 layout for Vulkan uniform buffer compatibility
layout(std140, set = 0, binding = 0) uniform SceneData {
    // === Camera Matrices (192 bytes) ===
    mat4 view;                              // offset 0,   size 64
    mat4 proj;                              // offset 64,  size 64
    mat4 viewproj;                          // offset 128, size 64

    // === Global Lighting (48 bytes) ===
    vec4 ambientColor;                      // offset 192, size 16 (rgb = color, a = intensity)
    vec4 sunlightDirection;                 // offset 208, size 16 (xyz = dir, w = intensity)
    vec4 sunlightColor;                     // offset 224, size 16

    // === Camera Info for Specular (16 bytes) ===
    vec4 cameraPosition;                    // offset 240, size 16 (xyz = pos, w = unused)

    // === Point Light Array (2048 bytes) ===
    PointLight pointLights[MAX_POINT_LIGHTS]; // offset 256, size 64 * 32 = 2048

    // === Point Light Count (16 bytes for alignment) ===
    int pointLightCount;                    // offset 2304, size 4
    float _pad0;                            // padding
    float _pad1;                            // padding
    float _pad2;                            // padding
} sceneData;

// =============================================================================
// TEXTURE BINDINGS
// =============================================================================

#ifdef USE_BINDLESS
layout(set = 0, binding = 1) uniform sampler2D allTextures[];
#else
layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
#endif

// Material data uniform buffer
layout(set = 1, binding = 0) uniform GLTFMaterialData {
    vec4 colorFactors;
    vec4 metal_rough_factors;
    int colorTexID;
    int metalRoughTexID;
} materialData;

#endif // INPUT_STRUCTURES_GLSL
