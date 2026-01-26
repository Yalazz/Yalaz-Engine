#version 450

// =============================================================================
// SHADED MODE FRAGMENT SHADER - Hemisphere lighting + N·L + Rim
// =============================================================================
// Provides a clean "studio lighting" look without textures
// Similar to Blender's Solid view mode

#define MAX_POINT_LIGHTS 64

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
    float _pad0, _pad1, _pad2;
} sceneData;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inWorldPos;

layout(location = 0) out vec4 outFragColor;

void main()
{
    vec3 N = normalize(inNormal);

    // === HEMISPHERE LIGHTING ===
    // Blend between cool sky (top) and warm ground (bottom)
    vec3 skyColor = vec3(0.85, 0.90, 0.95);   // Cool sky blue
    vec3 groundColor = vec3(0.35, 0.30, 0.25); // Warm brown
    float hemisphere = N.y * 0.5 + 0.5;        // Remap [-1,1] to [0,1]
    vec3 ambient = mix(groundColor, skyColor, hemisphere) * 0.35;

    // === STUDIO KEY LIGHT (top-front-right) ===
    vec3 keyLightDir = normalize(vec3(0.5, 0.8, 0.4));
    float NdotL = max(dot(N, keyLightDir), 0.0);
    vec3 diffuse = inColor * NdotL * 0.65;

    // === FILL LIGHT (opposite side, softer) ===
    vec3 fillLightDir = normalize(vec3(-0.6, 0.3, -0.3));
    float fillNdotL = max(dot(N, fillLightDir), 0.0);
    vec3 fill = inColor * fillNdotL * 0.2;

    // === RIM LIGHT (backlight for depth) ===
    vec3 viewDir = normalize(sceneData.cameraPosition.xyz - inWorldPos);
    float rim = 1.0 - max(dot(N, viewDir), 0.0);
    rim = pow(rim, 3.0) * 0.18;
    vec3 rimColor = vec3(rim);

    // === FINAL COMPOSITION ===
    vec3 result = ambient + diffuse + fill + rimColor;

    // Slight tone mapping to prevent harsh whites
    result = result / (result + vec3(0.5)) * 1.2;

    outFragColor = vec4(result, 1.0);
}
