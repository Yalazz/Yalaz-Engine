#version 460
#extension GL_GOOGLE_include_directive : require
#include "defines/fragdef.h"
#include "defines/defines.h"

void main()
{
    outfragcolor = texture(textures[GetTextureInd()], incoord.xy).xyzw;
}
