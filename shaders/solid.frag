#version 450

// =============================================================================
// SOLID MODE FRAGMENT SHADER - No lighting, flat color output
// =============================================================================

layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = inColor;
}
