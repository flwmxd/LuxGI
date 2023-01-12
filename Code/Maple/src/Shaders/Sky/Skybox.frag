//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450

layout(location = 0) in vec3 texCoords;
layout(location = 0) out vec4 outFrag;

layout(set = 0,binding = 1) uniform samplerCube uCubeMap;

layout(set = 0,binding = 2) uniform UniformBufferObjectLod {
    float lodLevel;
} ubo;


void main()
{    
    vec3 color = textureLod(uCubeMap, texCoords, ubo.lodLevel).rgb;
    outFrag = vec4(color, 1.0);
}