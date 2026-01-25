// =============================================================================
// MATERIAL PREVIEW VERTEX SHADER
// Uses buffer device address to read vertex data (same pattern as mesh.vert)
// =============================================================================
#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require
#include "input_structures.glsl"

// Outputs to fragment shader
layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;

// Vertex structure - must match C++ Vertex struct
struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

// Buffer reference for reading vertices
layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

// Push constants - must match C++ GPUDrawPushConstants
layout(push_constant) uniform constants {
    mat4 render_matrix;
    VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
    // Load vertex data from device address
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    // Transform position
    vec4 worldPos = PushConstants.render_matrix * vec4(v.position, 1.0);
    gl_Position = sceneData.viewproj * worldPos;

    // Output to fragment shader
    fragUV = vec2(v.uv_x, v.uv_y);
    fragNormal = normalize((PushConstants.render_matrix * vec4(v.normal, 0.0)).xyz);
    fragColor = v.color;
}
