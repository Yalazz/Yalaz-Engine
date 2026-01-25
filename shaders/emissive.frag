#version 450

// =============================================================================
// EMISSIVE FRAGMENT SHADER - For point light visualization glow effect
// =============================================================================

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec4 fragBaseColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Distance-based fade for glow effect
    float dist = length(fragWorldPos);

    // Inverse square falloff with clamping
    float fade = 1.0 / (dist * dist + 1.0);
    fade = clamp(fade, 0.0, 1.0);

    // Emissive glow effect
    vec3 glow = fragBaseColor.rgb * fade * 2.0;

    outColor = vec4(glow, fade);
}
