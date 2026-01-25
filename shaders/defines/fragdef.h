layout (location = 0) in vec4 inbase;
layout (location = 1) in vec4 inpos;
layout (location = 2) in vec3 innormal;
layout (location = 3) in vec3 incolor;
layout (location = 4) in vec2 incoord;
layout (location = 5) in vec4 inweight;
layout (location = 6) flat in ivec4 inboneid;

layout (location = 0) out vec4 outfragcolor;

//layout(set = 1, binding = 0) uniform sampler2D texturesampler;
