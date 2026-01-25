#extension GL_EXT_nonuniform_qualifier : enable

#define BindlessDescriptorSet 0

#define BindlessUniformBinding 0
#define BindlessStorageBinding 1
#define BindlessSamplerBinding 2

#define GetLayoutVariableName(Name) u##Name##Register

#define RegisterUniform(Name, Struct) \
    layout(set = BindlessDescriptorSet, binding = BindlessUniformBinding) \
        uniform Name Struct \
        GetLayoutVariableName(Name)[]

#define RegisterBuffer(Layout, BufferAccess, Name, Struct) \
    layout(Layout, set = BindlessDescriptorSet, binding = BindlessStorageBinding) \
    BufferAccess buffer Name Struct GetLayoutVariableName(Name)[]

#define RegisterBufferReadWrite(Layout, Name, Struct) \
    layout(Layout, set = BindlessDescriptorSet, binding = BindlessStorageBinding) \
    buffer Name Struct GetLayoutVariableName(Name)[]


#define GetResource(Name, Index) \
    GetLayoutVariableName(Name)[Index]

RegisterUniform(DummyUniform, {uint ignore; });
RegisterBuffer(std430, readonly, DummyBuffer, { uint ignore; });

layout(set = BindlessDescriptorSet, binding = BindlessSamplerBinding) \
    uniform sampler2D textures[];

layout( push_constant ) uniform constants
{
    int bindtexture;
    int bindstorage;
    int bindinstance;
    int bindbuffer;

    int objectID;
    int selected;
    int boneID;
    int gizmo;
} pushconstants;

int GetStorageInd() {
  return pushconstants.bindstorage;
}
int GetInstanceInd() {
  return pushconstants.bindinstance;
}
int GetTextureInd() {
  return pushconstants.bindtexture;
}
int GetUniformInd() {
  return pushconstants.bindbuffer;
}
RegisterUniform(Camera, {
    vec4 viewpos;
    vec4 mousepos;
    vec4 viewport;
    
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewproj;

    float time;
    float p0;
    float p1;
    float p2;
});

#define DEPTH_ARRAY_SCALE 512
#define INSTANCE_SIZE 20
const int MAX_BONES = 200;

#define GIZMO_X 0
#define GIZMO_Y 1
#define GIZMO_Z 2

struct InstanceData {
    mat4 bonesmatrices[MAX_BONES];
    mat4 rendermatrix;

    uint hasanimation;
    uint pad0;
    uint pad1;
    uint pad2;
};

RegisterBuffer(std140, readonly, Instance, {
      InstanceData instances[];
});

RegisterBuffer(std430, writeonly, Storage, {
    uint data[DEPTH_ARRAY_SCALE]; 
});

