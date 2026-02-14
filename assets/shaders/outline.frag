#version 450

#include "defines/fragdef.h"
#include "defines/defines.h"

#define LINE_WEIGHT 2.5
#
void main()
{
    if (inpos.x >= 0.996 || inpos.y <= -0.996) {
        discard;
    }

    vec2 uv = inpos.xy * 0.5 - 0.5;
    float w = GetResource(Camera, GetUniformInd()).viewport.x ;
    float h =  GetResource(Camera, GetUniformInd()).viewport.y ;

    float dx = (1.0 / w) * LINE_WEIGHT;
    float dy = (1.0 / h) * LINE_WEIGHT;

    vec2 uv_center = uv;
    vec2 uv_right = vec2(uv_center.x + dx, uv_center.y);
    vec2 uv_top = vec2(uv_center.x, uv_center.y - dx);
    vec2 uv_top_right = vec2(uv_center.x + dx, uv_center.y - dx);

    float center = texture(textures[GetTextureInd()], uv_center).r;
    float top = texture(textures[GetTextureInd()], uv_top).r;
    float right = texture(textures[GetTextureInd()], uv_right).r;
    float top_right = texture(textures[GetTextureInd()], uv_top_right).r;

    float dT = abs(center - top);
    float dR = abs(center - right);
    float dTR = abs(center - top_right);

    float delta = 0.0;
    delta = max(delta, dT);
    delta = max(delta, dR);
    delta = max(delta, dTR);

    vec4 outline = vec4(delta, delta, delta, 1.0);
    if (delta <= 0.0) {
      discard;
    }

    outfragcolor = outline ;
}
