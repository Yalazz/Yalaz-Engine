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
// MAIN - With PBR metallic/roughness and emission support
// =============================================================================

void main()
{
    // === BASE COLOR (ALBEDO) ===
    vec3 albedo = inColor * materialData.colorFactors.rgb;

#ifdef USE_BINDLESS
    vec4 texColor = texture(allTextures[materialData.colorTexID], inUV);
#else
    vec4 texColor = texture(colorTex, inUV);
#endif

    albedo *= texColor.rgb;

    // === PBR PARAMETERS ===
    float metallic = materialData.metal_rough_factors.x;
    float roughness = max(materialData.metal_rough_factors.y, 0.04); // Prevent div by zero

    // Sample metallic-roughness map (G=roughness, B=metallic in GLTF spec)
#ifndef USE_BINDLESS
    vec4 metalRoughSample = texture(metalRoughTex, inUV);
    roughness *= metalRoughSample.g;
    metallic *= metalRoughSample.b;
#endif

    // === NORMAL & VIEW DIRECTION ===
    vec3 N = normalize(inNormal);
    vec3 V = normalize(sceneData.cameraPosition.xyz - inWorldPos);

    // === AMBIENT LIGHTING ===
    vec3 ambient = albedo * sceneData.ambientColor.rgb * sceneData.ambientColor.a;

    // === DIRECTIONAL LIGHTING (SUN) - Simple PBR influence with shadows ===
    vec3 sunDir = normalize(-sceneData.sunlightDirection.xyz);
    float sunIntensity = sceneData.sunlightDirection.w;
    float NdotL = max(dot(N, sunDir), 0.0);

    // Calculate shadow factor
    float shadow = calculate_shadow(inWorldPos, N);

    // Fresnel at normal incidence
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Simple approximation: less diffuse for metals, sharper specular for smooth surfaces
    vec3 diffuse = albedo * (1.0 - metallic) * NdotL;

    // Specular with roughness influence
    vec3 H = normalize(V + sunDir);
    float NdotH = max(dot(N, H), 0.0);
    float specPower = mix(8.0, 256.0, 1.0 - roughness);
    float spec = pow(NdotH, specPower) * (1.0 - roughness) * 0.5;
    vec3 specular = mix(vec3(spec), albedo * spec, metallic);

    // Apply shadow to directional lighting
    vec3 directional = (diffuse + specular) * sceneData.sunlightColor.rgb * sunIntensity * shadow;

    // === POINT LIGHTING ===
    vec3 pointLighting = calculate_point_lights(inWorldPos, N, albedo, V);

    // === EMISSION ===
    vec3 emission = materialData.extra[0].rgb * materialData.extra[0].w;

    // === FINAL COMPOSITION ===
    vec3 result = ambient + directional + pointLighting + emission;

    // Reinhard tone mapping
    result = result / (result + vec3(1.0));

    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));

    // Alpha from material
    float alpha = materialData.colorFactors.a * texColor.a;

    outFragColor = vec4(result, alpha);
}
