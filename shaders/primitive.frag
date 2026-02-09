#version 450

// =============================================================================
// PRIMITIVE FRAGMENT SHADER - With Face Color Support + Full Lighting
// =============================================================================
// Supports per-face coloring based on normal direction
// Full lighting: Ambient, Directional (Sun), Point Lights, Specular (Blinn-Phong)
// =============================================================================

// Inputs from vertex shader
layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec2 fragUV;
layout(location = 4) flat in int fragFaceIndex;

// Output
layout(location = 0) out vec4 outColor;

// Push constants for primitive rendering - MUST match vertex shader and C++ struct
layout(push_constant) uniform PushConstants {
    mat4 worldMatrix;       // 64 bytes (offset 0)
    vec4 mainColor;         // 16 bytes (offset 64) - RGBA base color
    vec4 faceColors[6];     // 96 bytes (offset 80) - Per-face colors
    vec4 pbrParams;         // 16 bytes (offset 176) - x=metallic, y=roughness, z=ao, w=reflectionIntensity
    vec4 emission;          // 16 bytes (offset 192) - xyz=emission color, w=emission strength
    int useFaceColors;      // 4 bytes (offset 208)
    int padding[3];         // 12 bytes (offset 212)
} push;

// Scene data - MUST match GPUSceneData in vk_types.h
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
    // Point light shadow data
    vec4 pointLightShadowData[4];           // xyz = light pos, w = far plane (radius)
    int pointLightShadowCount;              // Number of shadow-casting point lights
    int _shadowPad1;                        // padding
    int _shadowPad2;                        // padding
    int _shadowPad3;                        // padding
    ivec4 pointLightShadowIndices;          // x,y,z,w = indices into pointLights array
} sceneData;

// Shadow map sampler
layout(set = 0, binding = 2) uniform sampler2D shadowMap;
// Point light shadow cubemaps
layout(set = 0, binding = 3) uniform samplerCube pointLightShadowMaps[4];
// Environment cubemap for reflections
layout(set = 0, binding = 4) uniform samplerCube envCubemap;

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

    // Smooth cascade transitions (reduce visible seams)
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

// =============================================================================
// MATERIAL TEXTURES (Set 1) - Same layout as GLTF materials
// =============================================================================

layout(set = 1, binding = 0) uniform GLTFMaterialData {
    vec4 colorFactors;           // Base color RGBA multiplier
    vec4 metal_rough_factors;    // x=metallic, y=roughness (texture multipliers)
    uint colorTexID;             // Bindless texture ID (unused in non-bindless mode)
    uint metalRoughTexID;        // Bindless texture ID (unused in non-bindless mode)
    uint pad1;
    uint pad2;
    vec4 extra[13];              // extra[0] = emission (xyz=color, w=strength)
} materialData;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;

// =============================================================================
// PBR LIGHTING CONSTANTS & FUNCTIONS
// =============================================================================

const float PI = 3.14159265359;

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// GGX/Trowbridge-Reitz normal distribution
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// Geometry function (Schlick-GGX)
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith's geometry function
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// =============================================================================
// PBR POINT LIGHT CALCULATION
// =============================================================================

vec3 calculate_point_lights_pbr(vec3 worldPos, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
    vec3 totalLight = vec3(0.0);

    // Calculate F0 (reflectance at normal incidence)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    int lightCount = sceneData.pointLightCount;
    if (lightCount <= 0) {
        return totalLight;
    }

    for (int i = 0; i < lightCount && i < MAX_POINT_LIGHTS; ++i)
    {
        PointLight light = sceneData.pointLights[i];

        vec3 lightVector = light.position - worldPos;
        float distance = length(lightVector);

        if (distance > light.radius) continue;

        vec3 L = normalize(lightVector);
        vec3 H = normalize(V + L);

        // Attenuation
        float attenuation = 1.0 - (distance / light.radius);
        attenuation = attenuation * attenuation;

        // === POINT LIGHT SHADOW ===
        float shadow = 1.0;
        int shadowIndex = get_point_light_shadow_index(i);
        if (shadowIndex >= 0) {
            shadow = calculate_point_light_shadow(shadowIndex, worldPos, light.position, light.radius);
        }

        vec3 radiance = light.color * light.intensity * attenuation * shadow;

        // Cook-Torrance BRDF
        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);
        totalLight += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    return totalLight;
}

// =============================================================================
// PBR DIRECTIONAL LIGHT (SUN) CALCULATION
// =============================================================================

vec3 calculate_directional_light_pbr(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
    vec3 L = normalize(-sceneData.sunlightDirection.xyz);
    vec3 H = normalize(V + L);
    float sunIntensity = sceneData.sunlightDirection.w;

    // Calculate F0 (reflectance at normal incidence)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 radiance = sceneData.sunlightColor.rgb * sunIntensity;

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// =============================================================================
// MAIN - PBR Rendering with Texture + Face Color Support
// =============================================================================

void main()
{
    // === SAMPLE TEXTURES ===
    vec4 texColor = texture(colorTex, fragUV);
    vec4 metalRoughSample = texture(metalRoughTex, fragUV);

    // === EXTRACT PBR PARAMETERS (push constants * texture) ===
    // GLTF spec: G channel = roughness, B channel = metallic
    float metallic = push.pbrParams.x * metalRoughSample.b;
    float roughness = max(push.pbrParams.y * metalRoughSample.g, 0.04);
    float ao = push.pbrParams.z;

    // === DETERMINE ALBEDO (texture * material factors * face/main color) ===
    vec3 texAlbedo = texColor.rgb * materialData.colorFactors.rgb;
    vec3 albedo;

    if (push.useFaceColors != 0 && fragFaceIndex >= 0 && fragFaceIndex < 6) {
        // Face colors MULTIPLY with texture (allows tinting textured surfaces)
        vec4 faceColor = push.faceColors[fragFaceIndex];
        albedo = texAlbedo * faceColor.rgb * push.mainColor.rgb;
    } else {
        albedo = texAlbedo * fragColor.rgb * push.mainColor.rgb;
    }

    // === NORMAL & VIEW DIRECTION ===
    vec3 N = normalize(fragNormal);

    // Handle back-facing normals
    if (!gl_FrontFacing) {
        N = -N;
    }

    vec3 V = normalize(sceneData.cameraPosition.xyz - fragWorldPos);

    // === AMBIENT LIGHTING (with AO) ===
    vec3 ambient = albedo * sceneData.ambientColor.rgb * sceneData.ambientColor.a * ao;

    // === SHADOW CALCULATION ===
    float shadow = calculate_shadow(fragWorldPos, N);

    // === PBR DIRECTIONAL LIGHTING (SUN) with shadows ===
    vec3 directional = calculate_directional_light_pbr(N, V, albedo, metallic, roughness) * shadow;

    // === PBR POINT LIGHTING ===
    vec3 pointLighting = calculate_point_lights_pbr(fragWorldPos, N, V, albedo, metallic, roughness);

    // === EMISSION (push constants + material data) ===
    vec3 emissionColor = push.emission.rgb;
    float emissionStrength = push.emission.w;
    // Also check material data emission
    vec3 matEmission = materialData.extra[0].rgb * materialData.extra[0].w;
    vec3 emission = emissionColor * emissionStrength + matEmission;

    // === ENVIRONMENT REFLECTION (controlled by push.pbrParams.w) ===
    float NdotV = max(dot(N, V), 0.0);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

    vec3 reflection = vec3(0.0);
    float reflectionIntensity = push.pbrParams.w; // 0 = no reflection, >0 = reflect
    if (reflectionIntensity > 0.0) {
        vec3 reflectDir = reflect(-V, N);
        vec3 envColor = texture(envCubemap, reflectDir).rgb;
        reflection = envColor * fresnel * (1.0 - roughness) * reflectionIntensity;
    }

    // === FINAL COMPOSITION ===
    vec3 result = ambient + directional + pointLighting + emission + reflection;

    // Reinhard tone mapping to prevent over-bright areas
    result = result / (result + vec3(1.0));

    // Gamma correction (linear to sRGB)
    result = pow(result, vec3(1.0 / 2.2));

    // Output with alpha from all sources
    float alpha = push.mainColor.a * texColor.a * materialData.colorFactors.a;
    if (push.useFaceColors != 0 && fragFaceIndex >= 0 && fragFaceIndex < 6) {
        alpha *= push.faceColors[fragFaceIndex].a;
    }

    // For transparent materials, increase alpha at grazing angles (Fresnel)
    if (alpha < 1.0) {
        float fresnelAlpha = 1.0 - pow(1.0 - NdotV, 3.0);
        alpha = mix(alpha, 1.0, (1.0 - fresnelAlpha) * 0.5);
    }

    outColor = vec4(result, alpha);
}
