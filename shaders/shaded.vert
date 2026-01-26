#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

// =============================================================================
// SHADED MODE VERTEX SHADER - Hemisphere lighting, no textures
// =============================================================================

// Scene data (Set 0, Binding 0) - full definition for camera position access
#define MAX_POINT_LIGHTS 64

struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;
};

layout(std140, set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
    vec4 cameraPosition;
    PointLight pointLights[MAX_POINT_LIGHTS];
    int pointLightCount;
    float _pad0, _pad1, _pad2;
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

layout(location = 0) out vec3 outWorldNormal;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec3 outWorldPos;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    vec4 worldPos = PushConstants.render_matrix * vec4(v.position, 1.0);

    gl_Position = sceneData.viewproj * worldPos;

    outWorldPos = worldPos.xyz;
    outWorldNormal = normalize((PushConstants.render_matrix * vec4(v.normal, 0.0)).xyz);
    outColor = v.color.rgb * PushConstants.baseColor.rgb;
}
