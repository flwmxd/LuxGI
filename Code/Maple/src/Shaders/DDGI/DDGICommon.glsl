//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#ifndef DDGI_COMMON
#define DDGI_COMMON

#include "../Raytraced/Random.glsl"
#include "../Common/Math.glsl"

struct DDGIUniform
{
    vec4  startPosition;
    vec4  step;
    ivec4 probeCounts;

    float maxDistance;
    float sharpness;
    float hysteresis;
    float normalBias;
    
    float ddgiGamma;
    int   irradianceProbeSideLength;
    int   irradianceTextureWidth;
    int   irradianceTextureHeight;
    
    int   depthProbeSideLength;
    int   depthTextureWidth;
    int   depthTextureHeight;
    int   raysPerProbe;
};

struct GIPayload
{
    vec3  L;
    vec3  T;
    float hitDistance;
    Random random;
};


vec3 sphericalFibonacci(float i,float raysPerProbe) 
{
    const float PHI = sqrt(5) * 0.5 + 0.5;

#define madfrac(A, B) ((A) * (B)-floor((A) * (B)))
    float phi       = 2.0 * PI * madfrac(i, PHI - 1);
    float cosTheta = 1.0 - (2.0 * i + 1.0) * (1.0 / raysPerProbe);
    float sinTheta = sqrt(clamp(1.0 - cosTheta * cosTheta, 0.0f, 1.0f));
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
#undef madfrac
}


float signNotZero(in float k)
{
    return (k >= 0.0) ? 1.0 : -1.0;
}

vec2 signNotZero(in vec2 v)
{
    return vec2(signNotZero(v.x), signNotZero(v.y));
}

vec2 octEncode(in vec3 v) 
{
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    vec2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0)
        result = (1.0 - abs(result.yx)) * signNotZero(result.xy);
    return result;
}

vec3 octDecode(vec2 o)
{
    vec3 v = vec3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));

    if (v.z < 0.0)
        v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);

    return normalize(v);
}

// Compute normalized oct coord, mapping top left of top left pixel to (-1,-1)
vec2 normalizedOctCoord(ivec2 fragCoord, int probeSideLength)
{
    int probeWithBorderSide = probeSideLength + 2;

    vec2 octFragCoord = ivec2((fragCoord.x - 2) % probeWithBorderSide, (fragCoord.y - 2) % probeWithBorderSide);
    // Add back the half pixel to get pixel center normalized coordinates
    return (vec2(octFragCoord) + vec2(0.5f)) * (2.0f / float(probeSideLength)) - vec2(1.0f, 1.0f);
}

int getProbeId(vec2 texel, int width, int probeSideLength)
{
    int probeWithBorderSide = probeSideLength + 2;
    int probesPerSide     = (width - 2) / probeWithBorderSide;
    return int(texel.x / probeWithBorderSide) + probesPerSide * int(texel.y / probeWithBorderSide);
}

vec3 gridToPosition(in DDGIUniform ddgi, ivec3 c)
{
    return ddgi.step.xyz * vec3(c) + ddgi.startPosition.xyz;
}

vec3 probeLocation(in DDGIUniform ddgi, int index)
{
    ivec3 gridCoord;
    gridCoord.x = index % ddgi.probeCounts.x;
    gridCoord.y = (index % (ddgi.probeCounts.x * ddgi.probeCounts.y)) / ddgi.probeCounts.x;
    gridCoord.z = index / (ddgi.probeCounts.x * ddgi.probeCounts.y);

    return gridToPosition(ddgi, gridCoord);
}

ivec3 probeIndexToGridCoord(in DDGIUniform ddgi, int index)
{
    ivec3 gridCoord;
    gridCoord.x = index % ddgi.probeCounts.x;
    gridCoord.y = (index % (ddgi.probeCounts.x * ddgi.probeCounts.y)) / ddgi.probeCounts.x;
    gridCoord.z = index / (ddgi.probeCounts.x * ddgi.probeCounts.y);
    return gridCoord;
}

ivec3 baseGridCoord(in DDGIUniform ddgi, vec3 X) 
{
    return clamp(ivec3((X - ddgi.startPosition.xyz) / ddgi.step.xyz), ivec3(0, 0, 0), ivec3(ddgi.probeCounts.xyz) - ivec3(1, 1, 1));
}

vec3 gridCoordToPosition(in DDGIUniform ddgi, ivec3 c)
{
    return ddgi.step.xyz * vec3(c) + ddgi.startPosition.xyz;
}

//Three dimension -> One dimension
int gridCoordToProbeIndex(in DDGIUniform ddgi, in ivec3 probeCoords) 
{
    return int(probeCoords.x + probeCoords.y * ddgi.probeCounts.x + probeCoords.z * ddgi.probeCounts.x * ddgi.probeCounts.y);
}

vec2 textureCoordFromDirection(vec3 dir, int probeIndex, int width, int height, int probeSideLength) 
{
    vec2 normalizedOctCoord = octEncode(normalize(dir));
    vec2 normalizedOctCoordZeroOne = (normalizedOctCoord + vec2(1.0f)) * 0.5f;

    float probeWithBorderSide = float(probeSideLength) + 2.0f;

    vec2 octCoordNormalizedToTextureDimensions = (normalizedOctCoordZeroOne * float(probeSideLength)) / vec2(float(width), float(height));

    int probesPerRow = (width - 2) / int(probeWithBorderSide); // how many probes in the texture altas

    vec2 probeTopLeftPosition = vec2(mod(probeIndex, probesPerRow) * probeWithBorderSide,
        (probeIndex / probesPerRow) * probeWithBorderSide) + vec2(2.0f, 2.0f);

    vec2 probeTopLeftPositionNormalized = vec2(probeTopLeftPosition) / vec2(float(width), float(height));

    return vec2(probeTopLeftPositionNormalized + octCoordNormalizedToTextureDimensions);
}


//P vertex.position
//N vertex.normal
vec3 sampleIrradiance(in DDGIUniform ddgi, vec3 P, vec3 N, vec3 Wo, sampler2D uIrradianceTexture, sampler2D uDepthTexture)
{
    ivec3 baseGridCoord = baseGridCoord(ddgi, P);
    vec3 baseProbePos   = gridCoordToPosition(ddgi, baseGridCoord);
    
    vec3  sumIrradiance = vec3(0.0f);
    float sumWeight = 0.0f;

    vec3 alpha = clamp((P - baseProbePos) / ddgi.step.xyz, vec3(0.0f), vec3(1.0f));

    for (int i = 0; i < 8; ++i) 
    {
        ivec3 offset = ivec3(i, i >> 1, i >> 2) & ivec3(1);
        ivec3 probeGridCoord = clamp(baseGridCoord + offset, ivec3(0), ddgi.probeCounts.xyz - ivec3(1));
        vec3 probePos = gridToPosition(ddgi, probeGridCoord);

        vec3 trilinear = mix(1.0 - alpha, alpha, offset);
        float weight = 1.0;

        // Smooth backface test
    
        vec3 dirToProbe = normalize(probePos - P);
        weight *= square(max(0.0001, (dot(dirToProbe, N) + 1.0) * 0.5)) + 0.2;

        int probeIdx = gridCoordToProbeIndex(ddgi, probeGridCoord);


        // Moment visibility test
        vec3 vBias = (N + 3.0 * Wo) * ddgi.normalBias;
        vec3 probeToPoint = P - probePos + vBias;
        vec3 dir = normalize(-probeToPoint);

        vec2 texCoord = textureCoordFromDirection(-dir, probeIdx, ddgi.depthTextureWidth, ddgi.depthTextureHeight, ddgi.depthProbeSideLength);

        float dist = length(probeToPoint);

        vec2 temp = textureLod(uDepthTexture, texCoord, 0.0f).rg;
        float mean = temp.x;
        float variance = abs(square(temp.x) - temp.y);

        float chebyshevWeight = variance / (variance + square(max(dist - mean, 0.0)));
            
        chebyshevWeight = max(chebyshevWeight * chebyshevWeight * chebyshevWeight, 0.0);

        weight *= (dist <= mean) ? 1.0 : chebyshevWeight;

        weight = max(0.000001, weight);
                 
        vec3 irradianceDir = N;

        texCoord = textureCoordFromDirection(normalize(irradianceDir), probeIdx, ddgi.irradianceTextureWidth, ddgi.irradianceTextureHeight, ddgi.irradianceProbeSideLength);

        vec3 probeIrradiance = textureLod(uIrradianceTexture, texCoord, 0.0f).rgb;
     
        probeIrradiance = pow(probeIrradiance, vec3(ddgi.ddgiGamma * 0.5f));

        const float crushThreshold = 0.2f;
        if (weight < crushThreshold)
            weight *= weight * weight * (1.0f / square(crushThreshold)); 

        // Trilinear weights
        weight *= trilinear.x * trilinear.y * trilinear.z;
        sumIrradiance += weight * probeIrradiance;
        sumWeight += weight;
    }

    vec3 netIrradiance = sumIrradiance / sumWeight;
    netIrradiance   *=netIrradiance; 

    return 2 * PI * netIrradiance;
}

#endif
