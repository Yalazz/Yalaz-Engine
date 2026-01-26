#version 450

// =============================================================================
// NORMALS MODE FRAGMENT SHADER - Visualize world-space normals as RGB
// =============================================================================
// X (right) = Red, Y (up) = Green, Z (forward) = Blue
// Normal range [-1,1] is mapped to color range [0,1]

layout(location = 0) in vec3 inNormal;
layout(location = 0) out vec4 outFragColor;

void main()
{
    // Normalize to handle interpolation artifacts
    vec3 N = normalize(inNormal);

    // Map from [-1, 1] to [0, 1]
    vec3 normalColor = N * 0.5 + 0.5;

    outFragColor = vec4(normalColor, 1.0);
}
