//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450

#extension GL_GOOGLE_include_directive : enable


layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inTileUV;
layout (location = 2) in uint inTileAddress;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec2 outTileUV;
layout (location = 2) out flat uint outAddress;

void main()
{
    outPosition = vec4(inPosition,1,1);
    outTileUV = inTileUV;
    outAddress = inTileAddress;
    gl_Position = outPosition;
}