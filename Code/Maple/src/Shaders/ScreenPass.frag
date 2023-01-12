//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#define GAMMA 2.2

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0)  uniform sampler2D uScreenSampler;
layout(set = 0, binding = 1) uniform UniformBuffer
{
	float gamma;
	float exposure;
	int toneMapIndex;
	int padding2;
} ubo;

layout(location = 0) out vec4 outFrag;

// Based on http://www.oscars.org/science-technology/sci-tech-projects/aces
vec3 ACESTonemap(vec3 color)
{
	mat3 m1 = mat3(
				   0.59719, 0.07600, 0.02840,
				   0.35458, 0.90834, 0.13383,
				   0.04823, 0.01566, 0.83777
				   );
	mat3 m2 = mat3(
				   1.60475, -0.10208, -0.00327,
				   -0.53108, 1.10813, -0.07276,
				   -0.07367, -0.00605, 1.07602
				   );
	vec3 v = m1 * color;
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return clamp(m2 * (a / b), 0.0, 1.0);
}

vec3 simpleReinhardToneMapping(vec3 color)
{
	float exposure = 1.5;
	color *= exposure/(1. + color / exposure);
	color = pow(color, vec3(1. / ubo.gamma));
	return color;
}

vec3 lumaBasedReinhardToneMapping(vec3 color)
{
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma / (1. + luma);
	color *= toneMappedLuma / luma;
	color = pow(color, vec3(1. / ubo.gamma));
	return color;
}

vec3 whitePreservingLumaBasedReinhardToneMapping(vec3 color)
{
	float white = 2.;
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma * (1. + luma / (white*white)) / (1. + luma);
	color *= toneMappedLuma / luma;
	color = pow(color, vec3(1. / ubo.gamma));
	return color;
}

vec3 RomBinDaHouseToneMapping(vec3 color)
{
    color = exp( -1.0 / ( 2.72*color + 0.15 ) );
	color = pow(color, vec3(1. / ubo.gamma));
	return color;
}

vec3 filmicToneMapping(vec3 color)
{
	color = max(vec3(0.), color - vec3(0.004));
	color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
	return color;
}

vec3 Uncharted2ToneMapping(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	float exposure = 2.;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	color = pow(color, vec3(1. / ubo.gamma));
	return color;
}


vec3 finalGamma(vec3 color)
{
	color = vec3(1.0) - exp(-color * ubo.exposure);
	return pow(color, vec3(1.0 / ubo.gamma));
}

vec3 fxaa()
{
	float FXAA_SPAN_MAX = 8.0;
    float FXAA_REDUCE_MUL = 1.0/8.0;
    float FXAA_REDUCE_MIN = (1.0/128.0);
   	vec2 texelSize = 1.0 / textureSize(uScreenSampler, 0);

    vec3 rgbNW = texture(uScreenSampler, inUV + (vec2(-1.0, -1.0) * texelSize)).xyz;
    vec3 rgbNE = texture(uScreenSampler, inUV + (vec2(+1.0, -1.0) * texelSize)).xyz;
    vec3 rgbSW = texture(uScreenSampler, inUV + (vec2(-1.0, +1.0) * texelSize)).xyz;
    vec3 rgbSE = texture(uScreenSampler, inUV + (vec2(+1.0, +1.0) * texelSize)).xyz;
    vec3 rgbM =  texture(uScreenSampler, inUV).xyz;

    vec3 luma    = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot( rgbM, luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
	      max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * texelSize;

    vec3 rgbA = (1.0/2.0) * (
		texture(uScreenSampler, inUV + dir * (1.0/3.0 - 0.5)).xyz +
		texture(uScreenSampler, inUV + dir * (2.0/3.0 - 0.5)).xyz
	);

    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
		texture(uScreenSampler, inUV + dir * (0.0/3.0 - 0.5)).xyz +
		texture(uScreenSampler, inUV + dir * (3.0/3.0 - 0.5)).xyz
	);
    float lumaB = dot(rgbB, luma);

    if((lumaB < lumaMin) || (lumaB > lumaMax))
		return rgbA;

	return rgbB;
}

void main()
{
	vec3 color = fxaa();

	int i = ubo.toneMapIndex;
	if (i == 1) color = finalGamma(color);
	if (i == 2) color = simpleReinhardToneMapping(color);
	if (i == 3) color = lumaBasedReinhardToneMapping(color);
	if (i == 4) color = whitePreservingLumaBasedReinhardToneMapping(color);
	if (i == 5) color = RomBinDaHouseToneMapping(color);		
	if (i == 6) color = filmicToneMapping(color);
	if (i == 7) color = Uncharted2ToneMapping(color);
	if (i == 8) color = ACESTonemap(color);
	
	outFrag = vec4(color, 1.0);
}