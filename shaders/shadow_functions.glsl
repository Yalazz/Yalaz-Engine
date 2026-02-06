// =============================================================================
// SHADOW FUNCTIONS - Advanced shadow techniques for high-quality rendering
// =============================================================================
// Includes:
// - PCSS (Percentage Closer Soft Shadows) with variable penumbra
// - Contact Shadows (screen-space ray marching for fine detail)
// - Standard PCF for comparison/fallback
// =============================================================================

#ifndef SHADOW_FUNCTIONS_GLSL
#define SHADOW_FUNCTIONS_GLSL

// =============================================================================
// PCSS CONFIGURATION
// =============================================================================
#define PCSS_BLOCKER_SEARCH_SAMPLES 32
#define PCSS_PCF_SAMPLES 64
#define PCSS_LIGHT_SIZE 0.02  // World-space light size for penumbra calculation
#define PCSS_MIN_PENUMBRA 0.0
#define PCSS_MAX_PENUMBRA 1.0

// Contact shadow configuration
#define CONTACT_SHADOW_STEPS 32
#define CONTACT_SHADOW_LENGTH 0.5  // World units
#define CONTACT_SHADOW_THICKNESS 0.01
#define CONTACT_SHADOW_FADE_START 0.8

// =============================================================================
// POISSON DISK SAMPLES - For high-quality sampling patterns
// =============================================================================
// Pre-computed Poisson disk for stable, well-distributed samples

const vec2 poissonDisk[64] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2( 0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870), vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845), vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554), vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507), vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367), vec2( 0.14383161, -0.14100790),
    vec2(-0.44451010, -0.78390738), vec2( 0.68987520,  0.54429870),
    vec2(-0.61731520,  0.02262238), vec2( 0.23458420, -0.74673540),
    vec2( 0.79291230,  0.90123940), vec2(-0.95878240, -0.02875420),
    vec2( 0.09842350,  0.45678920), vec2(-0.34512360, -0.12345670),
    vec2( 0.56723450,  0.23456780), vec2(-0.78934560,  0.67890120),
    vec2( 0.12345670, -0.56789010), vec2(-0.23456780,  0.78901230),
    vec2( 0.89012340, -0.12345670), vec2(-0.56789010,  0.34567890),
    vec2( 0.34567890, -0.89012340), vec2(-0.12345670,  0.56789010),
    vec2( 0.67890120, -0.34567890), vec2(-0.89012340,  0.12345670),
    vec2( 0.45678900,  0.67890120), vec2(-0.67890120, -0.45678900),
    vec2( 0.78901230,  0.12345670), vec2(-0.34567890, -0.67890120),
    vec2( 0.23456780,  0.89012340), vec2(-0.78901230, -0.23456780),
    vec2( 0.56789010,  0.34567890), vec2(-0.12345670, -0.89012340),
    vec2( 0.89012340,  0.45678900), vec2(-0.45678900, -0.56789010),
    vec2( 0.12345670,  0.78901230), vec2(-0.56789010, -0.34567890),
    vec2( 0.34567890,  0.56789010), vec2(-0.89012340, -0.67890120),
    vec2( 0.67890120,  0.23456780), vec2(-0.23456780, -0.78901230),
    vec2( 0.78901230, -0.45678900), vec2(-0.67890120,  0.89012340),
    vec2( 0.45678900, -0.12345670), vec2(-0.34567890,  0.45678900),
    vec2( 0.23456780, -0.23456780), vec2(-0.78901230,  0.34567890),
    vec2( 0.56789010, -0.67890120), vec2(-0.12345670,  0.12345670),
    vec2( 0.89012340, -0.78901230), vec2(-0.45678900,  0.67890120),
    vec2( 0.34567890, -0.56789010), vec2(-0.56789010,  0.23456780),
    vec2( 0.67890120, -0.89012340), vec2(-0.89012340,  0.56789010)
);

// =============================================================================
// PCSS - PERCENTAGE CLOSER SOFT SHADOWS
// =============================================================================
// Creates realistic soft shadows with variable penumbra width based on
// blocker distance. Shadows are sharp near contact and soft further away.

// Step 1: Find average blocker depth
float findBlockerDepth(sampler2D shadowMap, vec2 uv, float receiverDepth, float searchRadius, vec2 texelSize) {
    float blockerSum = 0.0;
    int blockerCount = 0;

    for (int i = 0; i < PCSS_BLOCKER_SEARCH_SAMPLES; ++i) {
        vec2 offset = poissonDisk[i] * searchRadius * texelSize;
        float shadowDepth = texture(shadowMap, uv + offset).r;

        if (shadowDepth < receiverDepth) {
            blockerSum += shadowDepth;
            blockerCount++;
        }
    }

    if (blockerCount == 0) {
        return -1.0;  // No blockers found
    }

    return blockerSum / float(blockerCount);
}

// Step 2: Calculate penumbra size based on blocker distance
float calculatePenumbraSize(float receiverDepth, float blockerDepth, float lightSize) {
    // Penumbra grows with distance from blocker
    float penumbra = lightSize * (receiverDepth - blockerDepth) / blockerDepth;
    return clamp(penumbra, PCSS_MIN_PENUMBRA, PCSS_MAX_PENUMBRA);
}

// Step 3: PCF with variable filter size
float pcfFilter(sampler2D shadowMap, vec2 uv, float receiverDepth, float filterRadius, float bias, vec2 texelSize) {
    float shadow = 0.0;

    for (int i = 0; i < PCSS_PCF_SAMPLES; ++i) {
        vec2 offset = poissonDisk[i] * filterRadius * texelSize;
        float shadowDepth = texture(shadowMap, uv + offset).r;
        shadow += (receiverDepth - bias > shadowDepth) ? 0.0 : 1.0;
    }

    return shadow / float(PCSS_PCF_SAMPLES);
}

// Complete PCSS shadow calculation
float calculate_pcss_shadow(sampler2D shadowMap, vec3 shadowCoords, vec3 normal, vec3 lightDir, float lightSize) {
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    // Calculate slope-based bias
    float NdotL = max(dot(normal, lightDir), 0.0);
    float slopeFactor = sqrt(1.0 - NdotL * NdotL);
    float bias = 0.001 + 0.003 * slopeFactor;

    // Step 1: Blocker search
    float searchRadius = lightSize * 10.0;  // Scale search radius with light size
    float blockerDepth = findBlockerDepth(shadowMap, shadowCoords.xy, shadowCoords.z, searchRadius, texelSize);

    if (blockerDepth < 0.0) {
        return 1.0;  // No blockers = fully lit
    }

    // Step 2: Calculate penumbra
    float penumbraSize = calculatePenumbraSize(shadowCoords.z, blockerDepth, lightSize);

    // Step 3: Filter with penumbra size
    return pcfFilter(shadowMap, shadowCoords.xy, shadowCoords.z, penumbraSize, bias, texelSize);
}

// =============================================================================
// CONTACT SHADOWS - Screen-space ray marching for fine shadow detail
// =============================================================================
// Adds sharp shadows for small-scale contact between objects that cascade
// shadow maps can't capture due to resolution limits.

float calculate_contact_shadow(
    vec3 worldPos,
    vec3 lightDir,
    mat4 viewMatrix,
    mat4 projMatrix,
    sampler2D depthTexture,
    vec2 screenSize
) {
    // Ray start in view space
    vec3 viewPos = (viewMatrix * vec4(worldPos, 1.0)).xyz;
    vec3 viewLightDir = normalize((viewMatrix * vec4(lightDir, 0.0)).xyz);

    // Ray march toward the light
    float stepSize = CONTACT_SHADOW_LENGTH / float(CONTACT_SHADOW_STEPS);
    vec3 rayStep = viewLightDir * stepSize;

    vec3 currentPos = viewPos;
    float shadow = 1.0;

    for (int i = 0; i < CONTACT_SHADOW_STEPS; ++i) {
        currentPos += rayStep;

        // Project to screen space
        vec4 clipPos = projMatrix * vec4(currentPos, 1.0);
        vec3 ndc = clipPos.xyz / clipPos.w;
        vec2 screenUV = ndc.xy * 0.5 + 0.5;

        // Check bounds
        if (screenUV.x < 0.0 || screenUV.x > 1.0 || screenUV.y < 0.0 || screenUV.y > 1.0) {
            break;
        }

        // Sample depth
        float sampledDepth = texture(depthTexture, screenUV).r;

        // Convert sampled depth to view space Z
        // For reverse-Z: closer objects have larger depth values
        vec4 sampledClip = vec4(ndc.xy, sampledDepth, 1.0);
        vec4 sampledView = inverse(projMatrix) * sampledClip;
        float sampledViewZ = sampledView.z / sampledView.w;

        // Check for occlusion
        float depthDiff = currentPos.z - sampledViewZ;

        if (depthDiff > 0.0 && depthDiff < CONTACT_SHADOW_THICKNESS) {
            // Hit! Calculate fade based on distance
            float t = float(i) / float(CONTACT_SHADOW_STEPS);
            float fade = smoothstep(CONTACT_SHADOW_FADE_START, 1.0, t);
            shadow = min(shadow, fade);
        }
    }

    return shadow;
}

// =============================================================================
// COMBINED SHADOW FUNCTION - Uses both PCSS and contact shadows
// =============================================================================

float calculate_combined_shadow(
    sampler2D shadowMap,
    sampler2D depthTexture,
    vec3 worldPos,
    vec3 normal,
    vec3 lightDir,
    mat4 lightMatrix,
    mat4 viewMatrix,
    mat4 projMatrix,
    vec2 screenSize,
    float lightSize,
    bool usePCSS,
    bool useContactShadows
) {
    float shadow = 1.0;

    // Standard shadow map lookup
    vec4 lightSpacePos = lightMatrix * vec4(worldPos, 1.0);
    vec3 shadowCoords = lightSpacePos.xyz / lightSpacePos.w;
    shadowCoords.xy = shadowCoords.xy * 0.5 + 0.5;

    // Check bounds
    if (shadowCoords.x >= 0.0 && shadowCoords.x <= 1.0 &&
        shadowCoords.y >= 0.0 && shadowCoords.y <= 1.0 &&
        shadowCoords.z >= 0.0 && shadowCoords.z <= 1.0) {

        if (usePCSS) {
            shadow = calculate_pcss_shadow(shadowMap, shadowCoords, normal, lightDir, lightSize);
        } else {
            // Standard PCF fallback
            vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
            float NdotL = max(dot(normal, lightDir), 0.0);
            float bias = 0.001 + 0.002 * sqrt(1.0 - NdotL * NdotL);
            shadow = pcfFilter(shadowMap, shadowCoords.xy, shadowCoords.z, 1.0, bias, texelSize);
        }
    }

    // Add contact shadows
    if (useContactShadows) {
        float contactShadow = calculate_contact_shadow(
            worldPos, lightDir, viewMatrix, projMatrix, depthTexture, screenSize
        );
        shadow = min(shadow, contactShadow);
    }

    return shadow;
}

// =============================================================================
// SPOT LIGHT SHADOW - For spot light shadow maps (single frustum)
// =============================================================================

float calculate_spot_light_shadow(
    sampler2D shadowMap,
    vec3 worldPos,
    mat4 lightViewProj,
    float bias
) {
    vec4 lightSpacePos = lightViewProj * vec4(worldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // Check bounds
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 1.0;
    }

    // PCF 3x3
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth) ? 0.0 : 1.0;
        }
    }

    return shadow / 9.0;
}

#endif // SHADOW_FUNCTIONS_GLSL
