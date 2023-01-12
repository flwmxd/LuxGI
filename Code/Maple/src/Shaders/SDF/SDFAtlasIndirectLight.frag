//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#include "AtlasCommon.glsl"
#include "SDFCommon.glsl"
#include "../Common/Light.glsl"
#include "../Common/Math.glsl"
#include "../DDGI/DDGICommon.glsl"
#include "../Common/Math.glsl"
#include "../Raytraced/Common.glsl"
#include "../Raytraced/BRDF.glsl"

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec2 inTileUV;
layout (location = 2) in flat uint inTileAddress;

// vec4(tile->x, tile->y, tileWidth, tileHeight) / (float) surface.resolution;
layout (set = 0, binding = 0, std430) buffer SDFAtlasTileBuffer{
    TileBuffer data[];
}atlasTiles;// start object index.

layout(set = 0, binding = 1) uniform sampler2D uIrradiance;
layout(set = 0, binding = 2) uniform sampler2D uDepth;
layout(set = 0, binding = 3, scalar) uniform DDGIUBO
{
    DDGIUniform ddgi;
};

layout (set = 0, binding = 4) uniform sampler2D uColorSampler;
layout (set = 0, binding = 5) uniform sampler2D uNormalSampler;
layout (set = 0, binding = 6) uniform sampler2D uPBRSampler;
layout (set = 0, binding = 7) uniform UniformBufferObject
{
   vec4 cameraPos;//w is intensity
} ubo;

layout (location = 0) out vec4 outColor;

vec3 indirectLighting(in SurfaceMaterial p, vec3 Wo, vec3 worldPos)
{
    vec3 diffuseColor = (p.albedo.rgb - p.albedo.rgb * p.metallic) / PI;
	return ubo.cameraPos.w * diffuseColor * sampleIrradiance(ddgi, worldPos, p.normal, Wo, uIrradiance, uDepth);
}

void main()
{
    TileBuffer tile = atlasTiles.data[inTileAddress];
    SurfaceMaterial p;
    vec2 atlasUV    = inTileUV * tile.extends.zw + tile.extends.xy;
    vec4 albedo     = texture(uColorSampler, atlasUV);
	vec4 normalTex	= texture(uNormalSampler, atlasUV);
	vec4 pbr        = texture(uPBRSampler,	atlasUV);

	p.normal        = octohedralToDirection(normalTex.xy);
    p.albedo        = vec4( min(albedo.xyz ,0.9) , 1); //energy loss..
    p.metallic      = pbr.r;
    p.roughness     = pbr.g;
    p.F0            = mix(vec3(0.03), p.albedo.xyz, p.metallic);
    
	vec3 worldPos   = vec3(albedo.w, normalTex.z, normalTex.w);
    const vec3 Wo   = normalize(ubo.cameraPos.xyz - worldPos ) ;
	outColor        = vec4(indirectLighting(p, Wo, worldPos), 1);
}