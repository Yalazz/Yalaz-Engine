#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(vec3(0.5, 0.5, -0.8));

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 baseColor = fragColor.rgb;

    outColor = vec4(baseColor * diff + 0.05, fragColor.a);
}
