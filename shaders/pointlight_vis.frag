#version 450

// =============================================================================
// POINT LIGHT VISUALIZATION FRAGMENT SHADER
// Creates a soft glowing sphere effect for light radius visualization
// =============================================================================

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec4 baseColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Calculate distance from center in local space (fragWorldPos is local)
    float dist = length(fragWorldPos);

    // Create soft edge glow - stronger at edges, fading toward center
    // This creates a "shell" effect showing the light boundary
    float edgeFactor = smoothstep(0.3, 1.0, dist);
    float innerFade = smoothstep(1.0, 0.7, dist);

    // Combine for a nice rim/shell effect
    float alpha = edgeFactor * innerFade * baseColor.a * 2.0;

    // Add slight pulsing fresnel-like effect at edges
    float rimGlow = pow(edgeFactor, 2.0) * 0.5;

    vec3 finalColor = baseColor.rgb + baseColor.rgb * rimGlow;

    outColor = vec4(finalColor, alpha);
}
