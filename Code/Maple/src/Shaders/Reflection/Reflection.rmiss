//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "ReflectionCommon.glsl"

layout (set = 0, binding = 4) uniform samplerCube uSkybox;

layout(location = 0) rayPayloadInEXT ReflectionPayload inPayload;

void main()
{
    inPayload.color = texture(uSkybox, gl_WorldRayDirectionEXT).rgb; 
    inPayload.rayLength = -1.0f;
}
