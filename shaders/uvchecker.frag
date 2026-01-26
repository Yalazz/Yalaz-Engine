#version 450

// =============================================================================
// UV CHECKER MODE FRAGMENT SHADER - Procedural checker pattern with UV tint
// =============================================================================
// Displays a checkerboard pattern to visualize UV mapping quality
// UV coordinates are shown as color gradient (U=Red, V=Green)

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outFragColor;

void main()
{
    // === CHECKERBOARD PATTERN ===
    float scale = 8.0;  // 8x8 checker grid per UV unit
    vec2 uv = inUV * scale;

    // Create checker pattern using floor modulo
    float checker = mod(floor(uv.x) + floor(uv.y), 2.0);

    // === BASE COLORS ===
    vec3 color1 = vec3(0.95, 0.95, 0.95);  // Near white
    vec3 color2 = vec3(0.15, 0.15, 0.15);  // Dark gray

    vec3 checkerColor = mix(color1, color2, checker);

    // === UV GRADIENT TINT ===
    // Show UV coordinates as color: U = Red, V = Green
    vec2 uvFract = fract(inUV);  // Use fractional part for 0-1 range
    vec3 uvTint = vec3(uvFract.x, uvFract.y, 0.3);

    // === SIMPLE SHADING for depth perception ===
    vec3 lightDir = normalize(vec3(0.5, 0.7, 0.5));
    float NdotL = max(dot(normalize(inNormal), lightDir), 0.0);
    float shading = 0.6 + NdotL * 0.4;  // 60% ambient + 40% diffuse

    // === FINAL COMPOSITION ===
    // Blend checker with UV tint, apply shading
    vec3 result = mix(checkerColor, uvTint, 0.35) * shading;

    outFragColor = vec4(result, 1.0);
}
