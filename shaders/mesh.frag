#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#include "input_structures.glsl"

// =============================================================================
// MESH FRAGMENT SHADER - With PBR, Normal Maps, Emissive Textures
// =============================================================================

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inWorldPos;
layout(location = 4) in vec4 inTangent;

layout(location = 0) out vec4 outFragColor;

// =============================================================================
// PBR CONSTANTS & FUNCTIONS
// =============================================================================

const float PI = 3.14159265359;

// GGX/Trowbridge-Reitz Normal Distribution Function
float DistributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// Smith's geometry function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness) {
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

// Schlick Fresnel approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Cook-Torrance specular BRDF for a single light
vec3 cook_torrance_brdf(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // Specular BRDF
    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    vec3  F = fresnelSchlick(HdotV, F0);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;

    // Energy conservation: diffuse reduced by fresnel (metallic surfaces have no diffuse)
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * NdotL;
}

// =============================================================================
// POINT LIGHT CALCULATION (Cook-Torrance PBR)
// =============================================================================

vec3 calculate_point_lights(vec3 worldPos, vec3 normal, vec3 albedo, vec3 viewDir, float metallic, float roughness, vec3 F0)
{
    vec3 totalLight = vec3(0.0);

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

        vec3 lightDir = normalize(lightVector);

        float attenuation = 1.0 - (distance / light.radius);
        attenuation = attenuation * attenuation;

        // Point light shadow
        float shadow = 1.0;
        int shadowIndex = get_point_light_shadow_index(i);
        if (shadowIndex >= 0) {
            shadow = calculate_point_light_shadow(shadowIndex, worldPos, light.position, light.radius);
        }

        vec3 brdf = cook_torrance_brdf(normal, viewDir, lightDir, albedo, metallic, roughness, F0);
        totalLight += brdf * light.color * light.intensity * attenuation * shadow;
    }

    return totalLight;
}

// =============================================================================
// TBN MATRIX CONSTRUCTION FOR NORMAL MAPPING
// =============================================================================

mat3 constructTBN(vec3 N, vec4 tangentData) {
    // Guard against zero-length tangent (causes NaN → black squares via bloom spread)
    float tangentLen = length(tangentData.xyz);
    if (tangentLen < 0.001) {
        // Generate a fallback tangent perpendicular to N
        vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        vec3 T = normalize(cross(up, N));
        vec3 B = cross(N, T);
        return mat3(T, B, N);
    }
    vec3 T = tangentData.xyz / tangentLen;
    // Re-orthogonalize T with respect to N
    T = T - dot(T, N) * N;
    float reorthoLen = length(T);
    if (reorthoLen < 0.001) {
        vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        T = normalize(cross(up, N));
    } else {
        T = T / reorthoLen;
    }
    vec3 B = cross(N, T) * tangentData.w; // Handedness
    return mat3(T, B, N);
}

// =============================================================================
// MAIN - With PBR, Normal Maps, Emissive Textures
// =============================================================================

void main()
{
    vec2 uv = inUV;
    vec3 preN = normalize(inNormal);
    vec3 preV = normalize(sceneData.cameraPosition.xyz - inWorldPos);
    preN = faceforward(preN, -preV, preN);
    uint displacementTexID = uint(max(materialData.extra[11].x, 0.0));

    if (displacementTexID > 0u) {
        float dispScale = materialData.extra[2].x;
        float dispBias = materialData.extra[2].y;
        if (abs(dispScale) > 0.0001) {
            mat3 preTBN = constructTBN(preN, inTangent);
            vec3 viewTS = transpose(preTBN) * preV;
            float heightSample = texture(allTextures[displacementTexID], uv).r;
            float parallax = (heightSample + dispBias - 0.5) * dispScale;
            uv += viewTS.xy * parallax;
        }
    }

    // === BASE COLOR (ALBEDO) ===
    vec3 albedo = inColor * materialData.colorFactors.rgb;

#ifdef USE_BINDLESS
    vec4 texColor = texture(allTextures[materialData.colorTexID], uv);
#else
    vec4 texColor = texture(colorTex, uv);
#endif

    // Linearize sRGB texture data (textures loaded as UNORM, not VK_FORMAT_*_SRGB)
    // Without this, lighting is computed on sRGB values → double gamma → washed out/too bright
    albedo *= pow(texColor.rgb, vec3(2.2));

    // === PBR PARAMETERS ===
    float metallic = materialData.metal_rough_factors.x;
    float roughness = max(materialData.metal_rough_factors.y, 0.04);

    // Sample metallic-roughness map (G=roughness, B=metallic in GLTF spec)
#ifdef USE_BINDLESS
    if (materialData.metalRoughTexID > 0u) {
        vec4 metalRoughSample = texture(allTextures[materialData.metalRoughTexID], uv);
        roughness *= metalRoughSample.g;
        metallic *= metalRoughSample.b;
    }
#else
    vec4 metalRoughSample = texture(metalRoughTex, uv);
    roughness *= metalRoughSample.g;
    metallic *= metalRoughSample.b;
#endif
    // Re-clamp after texture multiply (texture g=0 would make roughness=0 → NaN in BRDF)
    roughness = max(roughness, 0.04);
    metallic = clamp(metallic, 0.0, 1.0);

    // === NORMAL MAPPING ===
    vec3 N = normalize(inNormal);
    vec3 V = normalize(sceneData.cameraPosition.xyz - inWorldPos);
    // Keep lighting stable regardless pipeline front-face convention or mirrored winding.
    N = faceforward(N, -V, N);

#ifdef USE_BINDLESS
    // Apply normal map if available (normalTexID > 0)
    if (materialData.normalTexID > 0u) {
        mat3 TBN = constructTBN(N, inTangent);
        vec3 normalSample = texture(allTextures[materialData.normalTexID], uv).rgb;
        normalSample = normalSample * 2.0 - 1.0; // Convert from [0,1] to [-1,1]
        float normalStrength = materialData.metal_rough_factors.w;
        if (normalStrength <= 0.0) normalStrength = 1.0;
        normalSample.xy *= normalStrength;
        N = normalize(TBN * normalSample);
    }
#endif

    float NdotV = max(dot(N, V), 0.001);

    float ao = materialData.metal_rough_factors.z;
#ifdef USE_BINDLESS
    uint aoTexID = uint(max(materialData.extra[11].y, 0.0));
    if (aoTexID > 0u) {
        ao *= texture(allTextures[aoTexID], uv).r;
    }
#endif

    // === F0 (base reflectance) ===
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // === AMBIENT LIGHTING (hemisphere blend) ===
    vec3 ambientColor = sceneData.ambientColor.rgb * sceneData.ambientColor.a;
    // Simple hemisphere: top gets sky tint, bottom gets ground tint
    float hemiFactor = dot(N, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
    vec3 hemiAmbient = mix(ambientColor * 0.6, ambientColor, hemiFactor);
    // Metallic surfaces reflect ambient, dielectrics get diffuse ambient
    vec3 ambientDiffuse = albedo * hemiAmbient * (1.0 - metallic);
    vec3 ambientSpecular = F0 * hemiAmbient * 0.2;
    vec3 ambient = (ambientDiffuse + ambientSpecular) * ao;

    // === DIRECTIONAL LIGHTING (SUN) with Cook-Torrance PBR ===
    vec3 sunDir = normalize(-sceneData.sunlightDirection.xyz);
    float sunIntensity = sceneData.sunlightDirection.w;
    float shadow = calculate_shadow(inWorldPos, N);

    vec3 directional = cook_torrance_brdf(N, V, sunDir, albedo, metallic, roughness, F0)
                     * sceneData.sunlightColor.rgb * sunIntensity * shadow;

    // === POINT LIGHTING (Cook-Torrance PBR) ===
    vec3 pointLighting = calculate_point_lights(inWorldPos, N, albedo, V, metallic, roughness, F0);

    // === EMISSION ===
    vec3 emission = materialData.extra[0].rgb * materialData.extra[0].w;

#ifdef USE_BINDLESS
    // Sample emissive texture if available (linearize sRGB)
    if (materialData.emissiveTexID > 0u) {
        vec3 emissiveTex = texture(allTextures[materialData.emissiveTexID], uv).rgb;
        emission *= pow(emissiveTex, vec3(2.2));
    }
#endif

    // === MULTI-PROBE ENVIRONMENT REFLECTION ===
    vec3 reflection = vec3(0.0);
    float reflectionIntensity = materialData.extra[1].x;
    if (reflectionIntensity > 0.0) {
        vec3 fresnel = fresnelSchlick(NdotV, F0);
        vec3 reflectDir = reflect(-V, N);

        int probeCount = int(sceneData.probeSettings.x);
        vec3 skyColor = texture(envCubemaps[0], reflectDir).rgb;
        vec3 envColor = skyColor;

        if (probeCount > 0) {
            int closestProbe = -1;
            float closestDist = 1e10;
            for (int i = 0; i < probeCount && i < 4; ++i) {
                float dist = distance(inWorldPos, sceneData.probePositions[i].xyz);
                float radius = sceneData.probePositions[i].w;
                if (dist < closestDist && dist < radius) {
                    closestDist = dist;
                    closestProbe = i;
                }
            }
            if (closestProbe >= 0) {
                float radius = sceneData.probePositions[closestProbe].w;
                float weight = 1.0 - smoothstep(0.0, radius, closestDist);
                vec3 probeColor;
                if (closestProbe == 0)      probeColor = texture(envCubemaps[1], reflectDir).rgb;
                else if (closestProbe == 1) probeColor = texture(envCubemaps[2], reflectDir).rgb;
                else if (closestProbe == 2) probeColor = texture(envCubemaps[3], reflectDir).rgb;
                else                        probeColor = texture(envCubemaps[4], reflectDir).rgb;
                envColor = mix(skyColor, probeColor, weight);
            }
        }
        reflection = envColor * fresnel * (1.0 - roughness) * reflectionIntensity;
        // Prevent reflection-only bloom spikes when using reflection presets.
        reflection = min(reflection, vec3(1.0));
    }

    // === FINAL COMPOSITION (HDR linear output) ===
    // Tone mapping, gamma, contrast, saturation are applied in tonemap_final.comp
    // AFTER bloom, so emissive surfaces remain > 1.0 for bloom detection.
    vec3 result = ambient + directional + pointLighting + emission + reflection;

    // Alpha
    float alpha = materialData.colorFactors.a * texColor.a;

    // Alpha masking (alphaCutoff stored in extra[1].y, 0 = disabled)
    // GLTF MASK mode: pixels below cutoff are fully transparent (discarded),
    // pixels above cutoff are fully opaque (alpha = 1.0)
    float alphaCutoff = materialData.extra[1].y;
    if (alphaCutoff > 0.0) {
        if (alpha < alphaCutoff) {
            discard;
        }
        alpha = 1.0; // Masked materials are fully opaque where not discarded
    }

    // Keep GLTF alpha stable. Opacity should come from material/texture data only.

    // NaN/Inf safety: any NaN from PBR math gets spread by bloom into black rectangles
    result = clamp(result, vec3(0.0), vec3(65504.0));
    if (any(isnan(result)) || any(isinf(result))) {
        result = vec3(0.0);
    }

    outFragColor = vec4(result, alpha);
}
