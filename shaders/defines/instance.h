
void DefineInstanceResources()
{
    texturebind = GetResource(Instance, GetInstanceInd()).instances[gl_InstanceIndex].texturebind;
    storagebind = GetResource(Instance, GetInstanceInd()).instances[gl_InstanceIndex].storagebind;
    bufferbind = GetResource(Instance, GetInstanceInd()).instances[gl_InstanceIndex].bufferbind;
    objectID = GetResource(Instance, GetInstanceInd()).instances[gl_InstanceIndex].objectID;
    selected = GetResource(Instance, GetInstanceInd()).instances[gl_InstanceIndex].selected;
    boneID = GetResource(Instance, GetInstanceInd()).instances[gl_InstanceIndex].boneID;
    gizmo = GetResource(Instance, GetInstanceInd()).instances[gl_InstanceIndex].gizmo;
}
