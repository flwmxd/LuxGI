//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 worldPos;

layout(push_constant) uniform PushConsts 
{
	mat4 projView;
} pushConsts;

void main(void)
{
    worldPos 	= inPosition;
    gl_Position = pushConsts.projView * vec4(inPosition, 1.0f);
}
