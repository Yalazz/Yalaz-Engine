#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec4 inColor;  // Renk verisi

layout(location = 0) out vec4 fragColor;  // Fragmente renk geçişi

void main() {
    fragColor = inColor;  // Rengi geçiyoruz
    gl_Position = vec4(inPosition, 1.0);  // Vertex pozisyonu
}
