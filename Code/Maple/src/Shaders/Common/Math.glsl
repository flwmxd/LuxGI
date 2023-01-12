//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#ifndef MATH_H
#define MATH_H

#define PI 3.1415926535897932384626433832795

const float PHI = 1.61803398874989484820459;  // Φ = Golden Ratio   

float square(float v)
{
    return v * v;
}

vec3 square(vec3 v)
{
    return v * v;
}

vec2 directionToOctohedral(vec3 normal)
{
    vec2 p = normal.xy * (1.0f / dot(abs(normal), vec3(1.0f)));
    return normal.z > 0.0f ? p : (1.0f - abs(p.yx)) * (step(0.0f, p) * 2.0f - vec2(1.0f));
}

vec3 octohedralToDirection(vec2 e)
{
    vec3 v = vec3(e, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0.0)
        v.xy = (1.0 - abs(v.yx)) * (step(0.0, v.xy) * 2.0 - vec2(1.0));
    return normalize(v);
}

vec3 worldPositionFromDepth(vec2 texCoords, float ndcDepth, mat4 viewProjInv)
{
    vec2 screenPos = texCoords * 2.0 - 1.0;
    vec4 ndcPos = vec4(screenPos, ndcDepth, 1.0);
    vec4 worldPos = viewProjInv * ndcPos;
    worldPos = worldPos / worldPos.w;
    return worldPos.xyz;
}

#endif