//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#ifndef SAMPLING
#define SAMPLING

#include "Math.glsl"

vec3 importanceSampleCosine(vec2 rand, vec3 N)
{
    float phi = 2.f * PI * rand.y;

    float cosTheta = sqrt(rand.x);
    float sinTheta = sqrt(1 - rand.x);
    vec3 sampleHemisphere = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    //orient sample into world space
    vec3 up = abs(N.z) < 0.999 ? vec3(0.f, 0.f, 1.f) : vec3(1.f, 0.f, 0.f);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleWorld = vec3(0);
    sampleWorld += sampleHemisphere.x * tangent;
    sampleWorld += sampleHemisphere.y * bitangent;
    sampleWorld += sampleHemisphere.z * N;

    return sampleWorld;
}

#endif
