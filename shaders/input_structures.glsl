// =============================================================================
// INPUT STRUCTURES - GPU Shader Uniforms (std140 layout)
// =============================================================================
// CRITICAL: This file MUST match the C++ GPUSceneData struct in vk_types.h
// Any mismatch will cause lighting to fail silently!
// =============================================================================

#ifndef INPUT_STRUCTURES_GLSL
#define INPUT_STRUCTURES_GLSL

// Must match C++ MAX_POINT_LIGHTS and SHADOW_CASCADE_COUNT in vk_types.h
#define MAX_POINT_LIGHTS 64
#define SHADOW_CASCADE_COUNT 4

// Point light structure - 32 bytes (matches C++ PointLight)
// Layout: [position.xyz, radius] [color.xyz, intensity]
struct PointLight {
    vec3 position;      // offset 0,  size 12
    float radius;       // offset 12, size 4
    vec3 color;         // offset 16, size 12
    float intensity;    // offset 28, size 4
};  // Total: 32 bytes

// Scene data uniform buffer (set 0, binding 0)
// Total size: 2592 bytes (must match C++ GPUSceneData)
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

    // === Point Light Count + Shadow Settings (16 bytes) ===
    int pointLightCount;                    // offset 2304, size 4
    float shadowBias;                       // offset 2308, size 4
    float shadowNormalBias;                 // offset 2312, size 4
    int shadowsEnabled;                     // offset 2316, size 4

    // === Shadow Cascade Matrices (256 bytes) ===
    mat4 shadowMatrices[SHADOW_CASCADE_COUNT]; // offset 2320, size 64 * 4 = 256

    // === Shadow Cascade Split Depths (16 bytes) ===
    vec4 cascadeSplits;                     // offset 2576, size 16 (x,y,z,w = split distances)

    // === Point Light Shadow Data (96 bytes) ===
    vec4 pointLightShadowData[4];           // offset 2592, xyz = light pos, w = far plane (radius)
    int pointLightShadowCount;              // offset 2656, size 4
    int _shadowPad1;                        // offset 2660, padding
    int _shadowPad2;                        // offset 2664, padding
    int _shadowPad3;                        // offset 2668, padding
    ivec4 pointLightShadowIndices;          // offset 2672, size 16 (x,y,z,w = indices into pointLights)
} sceneData;

// =============================================================================
// SHADOW MAP BINDINGS
// =============================================================================
layout(set = 0, binding = 2) uniform sampler2D shadowMap;              // Directional light cascade shadow map
layout(set = 0, binding = 3) uniform samplerCube pointLightShadowMaps[4];  // Point light shadow cubemaps
layout(set = 0, binding = 4) uniform samplerCube envCubemap;              // Environment cubemap for reflections

// =============================================================================
// SHADOW CALCULATION FUNCTIONS
// =============================================================================

// =============================================================================
// CASCADE SHADOW MAPPING - Full implementation with PCF soft shadows
// =============================================================================
// Shadow map uses a 2x2 atlas layout:
//   [Cascade 0 | Cascade 1]
//   [Cascade 2 | Cascade 3]
// Each cascade covers progressively larger areas for distant objects

// Get UV offset and scale for a specific cascade in the atlas
vec2 getCascadeOffset(int cascade) {
    // 2x2 atlas layout
    return vec2(float(cascade % 2) * 0.5, float(cascade / 2) * 0.5);
}

// Determine the appropriate cascade based on view-space depth
int selectCascade(float viewDepth) {
    // Find the first cascade that contains this depth
    for (int i = 0; i < SHADOW_CASCADE_COUNT - 1; ++i) {
        if (viewDepth < sceneData.cascadeSplits[i]) {
            return i;
        }
    }
    return SHADOW_CASCADE_COUNT - 1;
}

// Calculate shadow factor with cascade selection and PCF filtering
float calculate_shadow(vec3 worldPos, vec3 normal) {
    if (sceneData.shadowsEnabled == 0) return 1.0;

    // Calculate view-space depth for cascade selection
    vec4 viewPos = sceneData.view * vec4(worldPos, 1.0);
    float viewDepth = -viewPos.z;

    // Select appropriate cascade based on distance from camera
    int cascadeIndex = selectCascade(viewDepth);

    // Transform to light space using the selected cascade matrix
    vec4 lightSpacePos = sceneData.shadowMatrices[cascadeIndex] * vec4(worldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Transform to [0,1] range for texture sampling
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // Check if outside valid shadow map range
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 1.0;  // Outside shadow map = fully lit
    }

    // Apply cascade atlas offset and scale
    vec2 atlasOffset = getCascadeOffset(cascadeIndex);
    vec2 atlasUV = projCoords.xy * 0.5 + atlasOffset;

    // Calculate slope-based bias to reduce shadow acne
    // Bias increases with surface angle to light
    vec3 lightDir = normalize(-sceneData.sunlightDirection.xyz);
    float NdotL = dot(normal, lightDir);
    float slopeFactor = sqrt(1.0 - NdotL * NdotL); // sin(angle)

    // Cascade-dependent bias (larger cascades need more bias)
    float cascadeBias = sceneData.shadowBias * (1.0 + float(cascadeIndex) * 0.5);
    float bias = cascadeBias + cascadeBias * slopeFactor * 2.0;

    // PCF 3x3 soft shadow sampling
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    // Scale texel size for atlas (each cascade is 0.5 of the texture)
    vec2 sampleStep = texelSize * 0.5;

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 sampleUV = atlasUV + vec2(x, y) * sampleStep;

            // Clamp to cascade bounds to prevent bleeding
            vec2 cascadeMin = atlasOffset;
            vec2 cascadeMax = atlasOffset + vec2(0.5);
            sampleUV = clamp(sampleUV, cascadeMin + sampleStep, cascadeMax - sampleStep);

            float pcfDepth = texture(shadowMap, sampleUV).r;
            shadow += (projCoords.z - bias > pcfDepth) ? 0.0 : 1.0;
        }
    }
    shadow /= 9.0;

    // Optional: Smooth cascade transitions (reduce visible seams)
    // Blend between cascades at the boundary
    float cascadeEnd = (cascadeIndex < SHADOW_CASCADE_COUNT - 1) ?
                        sceneData.cascadeSplits[cascadeIndex] :
                        sceneData.cascadeSplits[SHADOW_CASCADE_COUNT - 1];
    float blendStart = cascadeEnd * 0.9;

    if (viewDepth > blendStart && cascadeIndex < SHADOW_CASCADE_COUNT - 1) {
        // Sample next cascade for blending
        int nextCascade = cascadeIndex + 1;
        vec4 nextLightSpacePos = sceneData.shadowMatrices[nextCascade] * vec4(worldPos, 1.0);
        vec3 nextProjCoords = nextLightSpacePos.xyz / nextLightSpacePos.w;
        nextProjCoords.xy = nextProjCoords.xy * 0.5 + 0.5;

        if (nextProjCoords.x >= 0.0 && nextProjCoords.x <= 1.0 &&
            nextProjCoords.y >= 0.0 && nextProjCoords.y <= 1.0 &&
            nextProjCoords.z >= 0.0 && nextProjCoords.z <= 1.0) {

            vec2 nextAtlasOffset = getCascadeOffset(nextCascade);
            vec2 nextAtlasUV = nextProjCoords.xy * 0.5 + nextAtlasOffset;

            float nextCascadeBias = sceneData.shadowBias * (1.0 + float(nextCascade) * 0.5);
            float nextBias = nextCascadeBias + nextCascadeBias * slopeFactor * 2.0;

            float nextShadow = 0.0;
            for (int x = -1; x <= 1; ++x) {
                for (int y = -1; y <= 1; ++y) {
                    vec2 sampleUV = nextAtlasUV + vec2(x, y) * sampleStep;
                    vec2 cascadeMin = nextAtlasOffset;
                    vec2 cascadeMax = nextAtlasOffset + vec2(0.5);
                    sampleUV = clamp(sampleUV, cascadeMin + sampleStep, cascadeMax - sampleStep);

                    float pcfDepth = texture(shadowMap, sampleUV).r;
                    nextShadow += (nextProjCoords.z - nextBias > pcfDepth) ? 0.0 : 1.0;
                }
            }
            nextShadow /= 9.0;

            // Blend factor
            float blendFactor = (viewDepth - blendStart) / (cascadeEnd - blendStart);
            shadow = mix(shadow, nextShadow, blendFactor);
        }
    }

    return shadow;
}

// =============================================================================
// POINT LIGHT SHADOW CALCULATION
// =============================================================================
// Uses cubemap shadow maps for omnidirectional point light shadows

float calculate_point_light_shadow(int shadowIndex, vec3 worldPos, vec3 lightPos, float lightRadius) {
    if (sceneData.shadowsEnabled == 0 || shadowIndex < 0 || shadowIndex >= sceneData.pointLightShadowCount) {
        return 1.0;  // No shadow
    }

    // Direction from light to fragment (for cubemap sampling)
    vec3 fragToLight = worldPos - lightPos;
    float currentDepth = length(fragToLight);

    // Normalize for cubemap direction
    vec3 sampleDir = normalize(fragToLight);

    // Sample depth from cubemap (stores perspective depth)
    float sampledDepth;
    switch (shadowIndex) {
        case 0: sampledDepth = texture(pointLightShadowMaps[0], sampleDir).r; break;
        case 1: sampledDepth = texture(pointLightShadowMaps[1], sampleDir).r; break;
        case 2: sampledDepth = texture(pointLightShadowMaps[2], sampleDir).r; break;
        case 3: sampledDepth = texture(pointLightShadowMaps[3], sampleDir).r; break;
        default: return 1.0;
    }

    // Convert perspective depth to linear depth
    // Vulkan uses reverse-Z: depth = 1 at near, 0 at far
    // For perspective projection with near=0.1, far=lightRadius:
    // linearZ = (near * far) / (far - depth * (far - near))
    float nearPlane = 0.1;
    float farPlane = lightRadius;
    float closestDepth = (nearPlane * farPlane) / (farPlane - sampledDepth * (farPlane - nearPlane));

    // Bias based on distance (further = more bias needed)
    float bias = sceneData.shadowBias * (1.0 + currentDepth * 0.1);

    // Shadow test
    float shadow = (currentDepth - bias > closestDepth) ? 0.0 : 1.0;

    // PCF-like soft shadows using offset samples
    float shadowSum = shadow;
    float offset = 0.02;
    vec3 sampleOffsets[6] = vec3[](
        vec3( offset, 0, 0), vec3(-offset, 0, 0),
        vec3(0,  offset, 0), vec3(0, -offset, 0),
        vec3(0, 0,  offset), vec3(0, 0, -offset)
    );

    for (int i = 0; i < 6; i++) {
        vec3 offsetDir = normalize(fragToLight + sampleOffsets[i]);
        float rawSampleDepth;
        switch (shadowIndex) {
            case 0: rawSampleDepth = texture(pointLightShadowMaps[0], offsetDir).r; break;
            case 1: rawSampleDepth = texture(pointLightShadowMaps[1], offsetDir).r; break;
            case 2: rawSampleDepth = texture(pointLightShadowMaps[2], offsetDir).r; break;
            case 3: rawSampleDepth = texture(pointLightShadowMaps[3], offsetDir).r; break;
            default: rawSampleDepth = 1.0;
        }
        // Convert perspective depth to linear
        float sampleLinearDepth = (nearPlane * farPlane) / (farPlane - rawSampleDepth * (farPlane - nearPlane));
        shadowSum += (currentDepth - bias > sampleLinearDepth) ? 0.0 : 1.0;
    }

    return shadowSum / 7.0;  // Average of 7 samples
}

// Helper to get shadow index for a point light (returns -1 if no shadow)
int get_point_light_shadow_index(int lightIndex) {
    for (int i = 0; i < sceneData.pointLightShadowCount && i < 4; i++) {
        int shadowLightIndex = sceneData.pointLightShadowIndices[i];  // ivec4 allows [] indexing
        if (shadowLightIndex == lightIndex) {
            return i;
        }
    }
    return -1;
}

// =============================================================================
// TEXTURE BINDINGS
// =============================================================================

#ifdef USE_BINDLESS
layout(set = 0, binding = 5) uniform sampler2D allTextures[];
#else
layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
#endif

// Material data uniform buffer - MUST match C++ MaterialConstants struct
layout(set = 1, binding = 0) uniform GLTFMaterialData {
    vec4 colorFactors;           // Base color RGBA
    vec4 metal_rough_factors;    // x=metallic, y=roughness, z,w=unused
    uint colorTexID;             // Bindless texture ID for base color
    uint metalRoughTexID;        // Bindless texture ID for metallic-roughness
    uint pad1;
    uint pad2;
    vec4 extra[13];              // extra[0] = emission (xyz=color, w=strength)
                                 // extra[1].x = reflection intensity (0=none, 1=full)
                                 // extra[2-12] = reserved for future use
} materialData;

#endif // INPUT_STRUCTURES_GLSL
