//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 460

#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : require

#include "DDGICommon.glsl"


layout(location = 0) in vec3 inFragPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in flat int inProbeIdx;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D uIrradiance;
layout(set = 1, binding = 1) uniform sampler2D uDepth;
layout(set = 1, binding = 2, scalar) uniform DDGIUBO
{
    DDGIUniform ddgi;
};

layout(push_constant) uniform PushConstants
{
    float scale;
}pushConsts;

void main()
{
    vec2 probeCoord = textureCoordFromDirection(normalize(inNormal),
                                                    inProbeIdx,
                                                    ddgi.irradianceTextureWidth,
                                                    ddgi.irradianceTextureHeight,
                                                    ddgi.irradianceProbeSideLength);

    outColor = vec4(textureLod(uIrradiance, probeCoord, 0.0f).rgb, 1.0f);
}