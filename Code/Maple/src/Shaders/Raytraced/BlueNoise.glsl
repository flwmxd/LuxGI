//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#ifndef BLUE_NOISE_GLSL
#define BLUE_NOISE_GLSL

float sampleBlueNoise(ivec2 coord, int samplerIndex, int dimension, sampler2D sobalTex, sampler2D scramblingRanking)
{
	coord.x = coord.x % 128;
	coord.y = coord.y % 128;
	samplerIndex = samplerIndex % 256;
	dimension = dimension % 4;
	int rankedIndex = samplerIndex ^ int(clamp(texelFetch(scramblingRanking, ivec2(coord.x, coord.y), 0).b * 256.0f, 0.0f, 255.0f));
	int value = int(clamp(texelFetch(sobalTex, ivec2(rankedIndex, 0), 0)[dimension] * 256.0f, 0.0f, 255.0f));
	value = value ^ int(clamp(texelFetch(scramblingRanking, ivec2(coord.x, coord.y), 0)[dimension % 2] * 256.0f, 0.0f, 255.0f));
	return (0.5f + value) / 256.0f;
}

#endif