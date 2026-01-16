#version 450

// =============================================================================
// 2D/3D PRIMITIVE FRAGMENT SHADER - With Full Point Light Support
// =============================================================================
// This shader provides full lighting for primitives just like GLTF meshes
// Includes: Ambient, Directional (Sun), Point Lights, Specular (Blinn-Phong)
// =============================================================================

// Inputs from vertex shader
layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec2 fragUV;

// Output
layout(location = 0) out vec4 outColor;

// Push constants
layout(push_constant) uniform PushConstants {
    mat4 worldMatrix;
    uint vertexBufferAddress;
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

// =============================================================================
// LIGHTING CONSTANTS
// =============================================================================

const float PI = 3.14159265359;
const float SPECULAR_POWER = 32.0;
const float SPECULAR_STRENGTH = 0.5;

// =============================================================================
// POINT LIGHT CALCULATION
// =============================================================================

vec3 calculate_point_lights(vec3 worldPos, vec3 normal, vec3 baseColor, vec3 viewDir)
{
    vec3 totalLight = vec3(0.0);

    int lightCount = sceneData.pointLightCount;
    if (lightCount <= 0) {
        return totalLight;
    }

    for (int i = 0; i < lightCount && i < MAX_POINT_LIGHTS; ++i)
    {
        PointLight light = sceneData.pointLights[i];

        // Calculate light direction and distance
        vec3 lightVector = light.position - worldPos;
        float distance = length(lightVector);

        // Skip if outside light radius
        if (distance > light.radius) continue;

        vec3 lightDir = normalize(lightVector);

        // Quadratic attenuation for realistic falloff
        float attenuation = 1.0 - (distance / light.radius);
        attenuation = attenuation * attenuation;

        // === DIFFUSE LIGHTING ===
        float NdotL = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = baseColor * light.color * NdotL;

        // === SPECULAR LIGHTING (Blinn-Phong) ===
        vec3 halfVector = normalize(lightDir + viewDir);
        float NdotH = max(dot(normal, halfVector), 0.0);
        float specular = pow(NdotH, SPECULAR_POWER) * SPECULAR_STRENGTH;
        vec3 specularColor = light.color * specular;

        // Combine with intensity and attenuation
        vec3 lightContribution = (diffuse + specularColor) * light.intensity * attenuation;

        totalLight += lightContribution;
    }

    return totalLight;
}

// =============================================================================
// DIRECTIONAL LIGHT (SUN) CALCULATION
// =============================================================================

vec3 calculate_directional_light(vec3 normal, vec3 baseColor, vec3 viewDir)
{
    vec3 sunDir = normalize(-sceneData.sunlightDirection.xyz);
    float sunIntensity = sceneData.sunlightDirection.w;

    // Diffuse
    float NdotL = max(dot(normal, sunDir), 0.0);
    vec3 diffuse = baseColor * sceneData.sunlightColor.rgb * NdotL * sunIntensity;

    // Specular (Blinn-Phong)
    vec3 halfVector = normalize(sunDir + viewDir);
    float NdotH = max(dot(normal, halfVector), 0.0);
    float specular = pow(NdotH, SPECULAR_POWER) * SPECULAR_STRENGTH * 0.5;
    vec3 specularColor = sceneData.sunlightColor.rgb * specular * sunIntensity;

    return diffuse + specularColor;
}

// =============================================================================
// MAIN
// =============================================================================

void main()
{
    // === BASE COLOR ===
    vec3 baseColor = fragColor.rgb;

    // === NORMAL & VIEW DIRECTION ===
    vec3 normal = normalize(fragNormal);

    // Handle back-facing normals
    if (!gl_FrontFacing) {
        normal = -normal;
    }

    vec3 viewDir = normalize(sceneData.cameraPosition.xyz - fragWorldPos);

    // === AMBIENT LIGHTING ===
    vec3 ambient = baseColor * sceneData.ambientColor.rgb * sceneData.ambientColor.a;

    // === DIRECTIONAL LIGHTING (SUN) ===
    vec3 directional = calculate_directional_light(normal, baseColor, viewDir);

    // === POINT LIGHTING ===
    vec3 pointLighting = calculate_point_lights(fragWorldPos, normal, baseColor, viewDir);

    // === FINAL COMPOSITION ===
    vec3 result = ambient + directional + pointLighting;

    // Simple tone mapping to prevent over-bright areas
    result = result / (result + vec3(1.0));

    // Output with original alpha
    outColor = vec4(result, fragColor.a);
}
