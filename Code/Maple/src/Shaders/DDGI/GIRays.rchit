//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_ray_query : enable
#extension GL_EXT_debug_printf : enable

#include "DDGICommon.glsl"
#include "../Raytraced/Lighting.glsl"

layout(location = 0) rayPayloadInEXT GIPayload inPayload;

hitAttributeEXT vec2 hitAttribs;

layout (set = 0, binding = 0, std430) readonly buffer MaterialBuffer 
{
    Material data[];
} Materials;

layout (set = 0, binding = 1, std430) readonly buffer TransformBuffer 
{
    Transform data[];
} Transforms;


layout (set = 0, binding = 3) uniform accelerationStructureEXT uTopLevelAS;

layout (set = 0, binding = 4) uniform samplerCube uSkybox;


layout (set = 5, binding = 0) uniform sampler2D uIrradiance;
layout (set = 5, binding = 1) uniform sampler2D uDepth;
layout (set = 5, binding = 2, scalar) uniform DDGIUBO
{
    DDGIUniform ddgi;
};

layout(push_constant) uniform PushConstants
{
    mat4  randomOrientation;
    uint  numFrames;
    uint  infiniteBounces;
    int   numLights;
    float intensity;
}pushConsts;

#include "../Raytraced/RayQuery.glsl"
#include "../Raytraced/Sample.glsl"
#include "../Raytraced/Triangle.glsl"

void main()
{
    SurfaceMaterial surface;

    const Transform instance = Transforms.data[gl_InstanceCustomIndexEXT];
    const HitInfo hitInfo = getHitInfo();
    const Triangle triangle = getTriangle(instance, hitInfo);
    const Material material = Materials.data[hitInfo.materialIndex];

    const vec3 barycentrics = vec3(1.0 - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);

    surface.vertex = interpolatedVertex(triangle, barycentrics);

    transformVertex(instance, surface.vertex);

    getAlbedo(material, surface);
    getNormal(material, surface);
    getRoughness(material, surface);
    getMetallic(material, surface);
    getEmissive(material, surface);
    surface.roughness = max(surface.roughness, 0.00001);
    surface.F0 = mix(vec3(0.03), surface.albedo.xyz, surface.metallic);

    inPayload.L = vec3(0.0f);

    if (!isBlack(surface.emissive.rgb))
        inPayload.L += surface.emissive.rgb * surface.emissive.a;

    inPayload.L += directLighting(surface, pushConsts.numLights, uTopLevelAS);

    if (pushConsts.infiniteBounces == 1)
        inPayload.L += indirectLighting(surface,uIrradiance, uDepth,ddgi);

    inPayload.hitDistance = gl_RayTminEXT + gl_HitTEXT;
}
