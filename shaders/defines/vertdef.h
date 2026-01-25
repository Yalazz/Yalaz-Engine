layout (location = 0) in vec4 vinstbase;
layout (location = 1) in vec4 vposition;
layout (location = 2) in vec3 vnormal;
layout (location = 3) in vec3 vcolor;
layout (location = 4) in vec2 vuv;
layout (location = 5) in vec4 vweight;
layout (location = 6) in ivec4 vboneid;


layout (location = 0) out vec4 outbase;
layout (location = 1) out vec4 outpos;
layout (location = 2) out vec3 outnormal;
layout (location = 3) out vec3 outcolor;
layout (location = 4) out vec2 outcoord;
layout (location = 5) out vec4 outweight;
layout (location = 6) out ivec4 outboneid;
//
out gl_PerVertex {
    vec4 gl_Position;
};
