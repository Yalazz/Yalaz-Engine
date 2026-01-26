#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

// =============================================================================
// SOLID MODE VERTEX SHADER - No lighting, flat color output
// =============================================================================

// Scene data (Set 0, Binding 0)
layout(std140, set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
    vec4 cameraPosition;
    // Point lights array follows but not needed here
} sceneData;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(push_constant) uniform constants {
    mat4 render_matrix;
    VertexBuffer vertexBuffer;
    float outlineScale;
    float padding[5];
    vec4 baseColor;
} PushConstants;

layout(location = 0) out vec4 outColor;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    vec4 worldPos = PushConstants.render_matrix * vec4(v.position, 1.0);

    gl_Position = sceneData.viewproj * worldPos;

    // Output vertex color multiplied by push constant base color
    outColor = v.color * PushConstants.baseColor;
}
