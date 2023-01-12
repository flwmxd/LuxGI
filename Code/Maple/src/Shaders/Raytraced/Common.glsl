//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#ifndef COMMON_RAY_GLSL
#define COMMON_RAY_GLSL

#include "Random.glsl"

#define PATH_TRACE_CLOSEST_HIT_SHADER_IDX 0
#define PATH_TRACE_MISS_SHADER_IDX 0

#define VISIBILITY_CLOSEST_HIT_SHADER_IDX 1
#define VISIBILITY_MISS_SHADER_IDX 1
#define RADIANCE_CLAMP_COLOR vec3(1.0f)

#define M_PI 3.14159265359

struct Vertex
{
    vec3 position;
    vec4 color;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
};

struct Triangle
{
    Vertex v0;
    Vertex v1;
    Vertex v2;
};

struct Transform
{
    mat4 modelMatrix;
    mat4 normalMatrix;
    uint meshIdx;
    float padding[3];
};

struct HitInfo
{
    uint materialIndex;
    uint primitiveOffset;
    uint primitiveId;
};

struct PathTracePayload
{
    vec3 L;
    vec3 T;
    uint depth;
    Random random;
};

struct Material
{
    ivec4 textureIndices0;  // x: albedo, y: normals, z: roughness, w: metallic
    ivec4 textureIndices1;  // x: emissive, y: ao
    vec4  albedo;
    vec4  roughness;
    vec4  metallic;
    vec4  emissive; 
    vec4  usingValue0;        // albedo metallic roughness
	vec4  usingValue1;        // normal ao emissive workflow
};

struct SurfaceMaterial
{
    Vertex vertex;
    vec4 albedo;
    vec4 emissive;
    vec3 normal;
    vec3 F0;
    float metallic;
    float roughness;   
};

Vertex interpolatedVertex(in Triangle tri, in vec3 barycentrics)
{
    Vertex o;

    o.position = tri.v0.position.xyz * barycentrics.x + tri.v1.position.xyz * barycentrics.y + tri.v2.position.xyz * barycentrics.z;
    o.texCoord.xy = tri.v0.texCoord.xy * barycentrics.x + tri.v1.texCoord.xy * barycentrics.y + tri.v2.texCoord.xy * barycentrics.z;
    o.normal.xyz = normalize(tri.v0.normal.xyz * barycentrics.x + tri.v1.normal.xyz * barycentrics.y + tri.v2.normal.xyz * barycentrics.z);
    o.tangent.xyz = normalize(tri.v0.tangent.xyz * barycentrics.x + tri.v1.tangent.xyz * barycentrics.y + tri.v2.tangent.xyz * barycentrics.z);

    return o;
}

bool isBlack(vec3 c)
{
    return c.x == 0.0f && c.y == 0.0f && c.z == 0.0f;
}

vec4 gammaCorrectTexture(vec4 samp)
{
	return vec4(pow(samp.rgb, vec3(2.2)), samp.a);
}

#endif