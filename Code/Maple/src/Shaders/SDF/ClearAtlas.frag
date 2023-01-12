//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450

#extension GL_GOOGLE_include_directive : enable

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec2 inTileUV;
layout (location = 2) in flat uint inTileAddress;

layout(location = 0) out vec4 outColor0;
layout(location = 1) out vec4 outColor1;
layout(location = 2) out vec4 outColor2;
layout(location = 3) out vec4 outEmissive;

void main()
{
    outColor0 = vec4(0,0,0,0);
    outColor1 = vec4(0,0,0,0);
    outColor2 = vec4(0,0,0,0);
    outEmissive = vec4(0,0,0,0);
}