#version 450
#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

// =============================================================================
// MESH FRAGMENT SHADER - With Point Light Support
// =============================================================================

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inWorldPos;

layout(location = 0) out vec4 outFragColor;

// =============================================================================
// LIGHTING CONSTANTS
// =============================================================================

const float PI = 3.14159265359;
const float SPECULAR_POWER = 32.0;      // Shininess exponent
const float SPECULAR_STRENGTH = 0.5;    // Specular intensity multiplier


// =============================================================================
// POINT LIGHT CALCULATION
// =============================================================================
// Calculates diffuse and specular contribution from all active point lights

vec3 calculate_point_lights(vec3 worldPos, vec3 normal, vec3 baseColor, vec3 viewDir)
{
    vec3 totalLight = vec3(0.0);

    // Early exit if no point lights
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

        // Simple linear attenuation for debugging
        float attenuation = 1.0 - (distance / light.radius);
        attenuation = attenuation * attenuation; // Quadratic falloff

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
    float specular = pow(NdotH, SPECULAR_POWER) * SPECULAR_STRENGTH * 0.5; // Reduced for sun
    vec3 specularColor = sceneData.sunlightColor.rgb * specular * sunIntensity;

    return diffuse + specularColor;
}

// =============================================================================
// MAIN
// =============================================================================

void main()
{
    // === BASE COLOR ===
    vec3 baseColor = inColor;

#ifdef USE_BINDLESS
    vec4 texColor = texture(allTextures[materialData.colorTexID], inUV);
#else
    vec4 texColor = texture(colorTex, inUV);
#endif

    baseColor *= texColor.rgb;

    // === NORMAL & VIEW DIRECTION ===
    vec3 normal = normalize(inNormal);
    vec3 viewDir = normalize(sceneData.cameraPosition.xyz - inWorldPos);

    // === AMBIENT LIGHTING ===
    vec3 ambient = baseColor * sceneData.ambientColor.rgb * sceneData.ambientColor.a;

    // === DIRECTIONAL LIGHTING (SUN) ===
    vec3 directional = calculate_directional_light(normal, baseColor, viewDir);

    // === POINT LIGHTING ===
    vec3 pointLighting = calculate_point_lights(inWorldPos, normal, baseColor, viewDir);

    // === FINAL COMPOSITION ===
    vec3 result = ambient + directional + pointLighting;

    // Simple tone mapping to prevent over-bright areas
    result = result / (result + vec3(1.0));

    // DEBUG: Visualize point light count (uncomment to debug)
    // If you see magenta tint, point lights ARE being read from buffer
    // float debugLightCount = float(sceneData.pointLightCount) / 10.0;
    // result = mix(result, vec3(debugLightCount, 0.0, debugLightCount), 0.3);

    outFragColor = vec4(result, 1.0);
}
