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
    vec4 pbrParams;         // 16 bytes (offset 176) - x=metallic, y=roughness, z=ao, w=unused
    vec4 emission;          // 16 bytes (offset 192) - xyz=emission color, w=emission strength
    int useFaceColors;      // 4 bytes (offset 208)
    int padding[3];         // 12 bytes (offset 212)
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

        vec3 radiance = light.color * light.intensity * attenuation;

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
// MAIN - PBR Rendering with Emission Support
// =============================================================================

void main()
{
    // === EXTRACT PBR PARAMETERS ===
    float metallic = push.pbrParams.x;
    float roughness = max(push.pbrParams.y, 0.04); // Clamp roughness to avoid division issues
    float ao = push.pbrParams.z;

    // === DETERMINE ALBEDO (BASE COLOR) ===
    vec3 albedo;

    if (push.useFaceColors != 0 && fragFaceIndex >= 0 && fragFaceIndex < 6) {
        vec4 faceColor = push.faceColors[fragFaceIndex];
        albedo = faceColor.rgb * push.mainColor.rgb;
    } else {
        albedo = fragColor.rgb * push.mainColor.rgb;
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

    // === PBR DIRECTIONAL LIGHTING (SUN) ===
    vec3 directional = calculate_directional_light_pbr(N, V, albedo, metallic, roughness);

    // === PBR POINT LIGHTING ===
    vec3 pointLighting = calculate_point_lights_pbr(fragWorldPos, N, V, albedo, metallic, roughness);

    // === EMISSION ===
    vec3 emissionColor = push.emission.rgb;
    float emissionStrength = push.emission.w;
    vec3 emission = emissionColor * emissionStrength;

    // === FINAL COMPOSITION ===
    vec3 result = ambient + directional + pointLighting + emission;

    // Reinhard tone mapping to prevent over-bright areas
    result = result / (result + vec3(1.0));

    // Gamma correction (linear to sRGB)
    result = pow(result, vec3(1.0 / 2.2));

    // Output with alpha from main color
    float alpha = push.mainColor.a;
    if (push.useFaceColors != 0 && fragFaceIndex >= 0 && fragFaceIndex < 6) {
        alpha *= push.faceColors[fragFaceIndex].a;
    }

    outColor = vec4(result, alpha);
}
