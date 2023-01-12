//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450

#extension GL_GOOGLE_include_directive : enable

#include "AtlasCommon.glsl"

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec2 inTileUV;
layout (location = 2) in flat uint inTileAddress;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, std430) buffer SDFAtlasTileBuffer{
    TileBuffer data[];
}atlasTiles;// start object index.

layout(set = 0, binding = 1) uniform sampler2D uEmissiveSampler;

void main()
{
    TileBuffer tile = atlasTiles.data[inTileAddress];
    vec2 atlasUV = inTileUV * tile.extends.zw + tile.extends.xy;
    outColor = texture(uEmissiveSampler,atlasUV);
}