//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450

#extension GL_GOOGLE_include_directive : enable

#include "AtlasCommon.glsl"
#include "SDFCommon.glsl"
#include "../Common/Light.glsl"
#include "../Common/Math.glsl"
#include "../Common/GBufferSampler.glsl"

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec2 inTileUV;
layout (location = 2) in flat uint inTileAddress;

layout (set = 0, binding = 7) uniform sampler3D uGlobalMipSDF;
layout (set = 0, binding = 8) uniform sampler3D uGlobalSDF;

// vec4(tile->x, tile->y, tileWidth, tileHeight) / (float) surface.resolution;
layout (set = 0, binding = 0, std430) buffer SDFAtlasTileBuffer{
    TileBuffer data[];
}atlasTiles;// start object index.

layout (set = 0, binding = 1) uniform sampler2D uColorSampler;
layout (set = 0, binding = 2) uniform sampler2D uNormalSampler;
layout (set = 0, binding = 3) uniform sampler2D uDepthSampler;
layout (set = 0, binding = 4) uniform sampler2D uPBRSampler;
layout (set = 0, binding = 5) uniform sampler2D uEmissiveSampler;
layout (set = 0, binding = 6) uniform UniformBufferObject
{
   vec4 cameraPos;//w is shader bias
   Light light;
   GlobalSDFData data;
} ubo;

layout (location = 0) out vec4 outColor;


void fetchLight(in Light light, vec3 worldPos, vec3 normal, out vec3 Wi, out float tmax, out float attenuation)
{
    vec3 lightDir = vec3(0,0,0);
    float lightRadius = 0;

	if (light.type == LIGHT_DIRECTIONAL)
    {
        lightDir = -light.direction.xyz;
        tmax = GLOBAL_SDF_WORLD_SIZE;
        Wi = lightDir;
        attenuation = 1.0;
    } 
    else if(light.type == LIGHT_POINT)
    {
        vec3 dir = light.position.xyz - worldPos;
        float dist = length(dir);
        lightDir = normalize(dir);
        attenuation = light.radius / (pow(dist, 2.0) + 1.0);
        Wi = lightDir;
        tmax = dist;
    }
    else if(light.type == LIGHT_SPOT)
    {
        vec3 L = light.position.xyz -  worldPos;
        float cutoffAngle   = 1.0f - light.angle;      
        lightDir            = normalize(L);
        float dist          = length(L);
        float theta         = dot(lightDir.xyz, light.direction.xyz);
        float epsilon       = cutoffAngle - cutoffAngle * 0.9f;
        attenuation 	    = ((theta - cutoffAngle) / epsilon); // atteunate when approaching the outer cone
        attenuation         *= light.radius / (pow(dist, 2.0) + 1.0);//saturate(1.0f - dist / light.range);
        attenuation         = clamp(attenuation, 0.0, 1.0);
        Wi = lightDir;
        tmax = dist;
    }
}


void main()
{
    TileBuffer tile = atlasTiles.data[inTileAddress];
    vec2 atlasUV    = inTileUV * tile.extends.zw + tile.extends.xy;
    vec4 albedo     = texture(uColorSampler, atlasUV);
	vec4 normalTex	= texture(uNormalSampler, atlasUV);
	vec4 pbr        = texture(uPBRSampler,	atlasUV);
	vec3 normal     = octohedralToDirection(normalTex.xy);
	vec3 worldPos   = vec3(albedo.w, normalTex.z, normalTex.w);

    vec3 L = vec3(0);
    float dist = GLOBAL_SDF_WORLD_SIZE;
    float atten = 1;

    fetchLight(ubo.light, worldPos, normal,L,dist,atten);

    float shadowMask = 1.0;
//default SDF Shadow
    float NoL = dot(normal, L);
    float shadowBias = ubo.cameraPos.w;
    float bias = 2 * shadowBias * clamp(1 - NoL,0,1) + shadowBias;
    if (NoL > 0)
    {
        if( atten > 0 )
        {
            GlobalSDFTrace trace;
            trace.worldPosition = worldPos + normal * shadowBias;
            trace.worldDirection = L;
            trace.minDistance = bias;
            trace.maxDistance = dist - bias;
            trace.stepScale = 1;
            trace.needsHitNormal = false;
            GlobalSDFHit hit = tracyGlobalSDF(ubo.data, uGlobalSDF, uGlobalMipSDF, trace, 2);
            shadowMask = isHit(hit) ? 0.0 : 1;
        }
    }
    else
    {
         shadowMask = 0;
    }

    float metallic = pbr.r;
    float roughness= pbr.g;

    vec3 view = normalize( ubo.cameraPos.xyz - worldPos);
    float intensity = pow( ubo.light.intensity,1.4) + 0.1;
    vec3 Lradiance = ubo.light.color.xyz * intensity ;
    vec3 Lh = normalize(L + view);
    float cosLi = max(0.0, dot(normal, L));
    vec3 directShading = BRDF(albedo.rgb, normal, roughness,metallic, view, Lh, L) * Lradiance * cosLi * shadowMask * atten;
    outColor = vec4(directShading,1.0);
}