#version 450

layout (location = 0) in vec2 fragUV;
layout (location = 2) in vec4 fragColor;

layout (set = 1, binding = 1) uniform sampler2D colorTexture;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = texture(colorTexture, fragUV) * fragColor;
}
