#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require
#include "input_structures.glsl"

layout(location = 0) out vec3 outWorldNormal;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec3 outWorldPos;
layout(location = 4) out vec4 outTangent;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
    vec4 tangent;
};

struct SkinVertex {
    ivec4 joints;
    vec4 weights;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(buffer_reference, std430) readonly buffer SkinBuffer {
    SkinVertex skin[];
};

layout(buffer_reference, std430) readonly buffer BoneBuffer {
    mat4 bones[];
};

layout(push_constant) uniform constants {
    mat4 render_matrix;
    VertexBuffer vertexBuffer;
    SkinBuffer skinBuffer;
    BoneBuffer boneBuffer;
} PushConstants;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    SkinVertex sv = PushConstants.skinBuffer.skin[gl_VertexIndex];

    mat4 skinMat =
        sv.weights.x * PushConstants.boneBuffer.bones[sv.joints.x] +
        sv.weights.y * PushConstants.boneBuffer.bones[sv.joints.y] +
        sv.weights.z * PushConstants.boneBuffer.bones[sv.joints.z] +
        sv.weights.w * PushConstants.boneBuffer.bones[sv.joints.w];

    vec4 localPos = skinMat * vec4(v.position, 1.0);
    vec3 localNrm = normalize(mat3(skinMat) * v.normal);
    vec3 localTan = normalize(mat3(skinMat) * v.tangent.xyz);

    vec4 worldPos = PushConstants.render_matrix * localPos;
    gl_Position = sceneData.viewproj * worldPos;

    outWorldPos = worldPos.xyz;
    outColor = v.color.rgb;
    outUV = vec2(v.uv_x, v.uv_y);

    mat3 normalMatrix = transpose(inverse(mat3(PushConstants.render_matrix)));
    vec3 worldNormal = normalMatrix * localNrm;
    float normalLen = length(worldNormal);
    outWorldNormal = (normalLen > 0.0001) ? (worldNormal / normalLen) : vec3(0.0, 1.0, 0.0);

    vec3 worldTangent = normalize((PushConstants.render_matrix * vec4(localTan, 0.0)).xyz);
    outTangent = vec4(worldTangent, v.tangent.w);
}
