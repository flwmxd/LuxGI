//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 460

#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : require

#include "DDGICommon.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;


layout(location = 0) out vec3 outFragPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out flat int outProbeIdx;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 viewProj;
}ubo;

layout(set = 1, binding = 2, scalar) uniform DDGIUBO
{
    DDGIUniform ddgi;
};

layout(push_constant) uniform PushConstants
{
    float scale;
}pushConsts;

// ------------------------------------------------------------------------
// MAIN -------------------------------------------------------------------
// ------------------------------------------------------------------------

void main()
{
    ivec3 gridCoord = probeIndexToGridCoord(ddgi, gl_InstanceIndex);

    vec3 probePosition = gridCoordToPosition(ddgi, gridCoord);

    gl_Position = ubo.viewProj * vec4((inPosition * pushConsts.scale) + probePosition, 1.0f);

    outNormal   = inNormal;
    outProbeIdx = gl_InstanceIndex;
}
