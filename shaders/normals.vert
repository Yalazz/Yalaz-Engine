#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

// =============================================================================
// NORMALS MODE VERTEX SHADER - Pass world-space normals for visualization
// =============================================================================

layout(std140, set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
    vec4 cameraPosition;
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
} PushConstants;

layout(location = 0) out vec3 outWorldNormal;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    vec4 worldPos = PushConstants.render_matrix * vec4(v.position, 1.0);

    gl_Position = sceneData.viewproj * worldPos;

    // Transform normal to world space
    outWorldNormal = normalize((PushConstants.render_matrix * vec4(v.normal, 0.0)).xyz);
}
