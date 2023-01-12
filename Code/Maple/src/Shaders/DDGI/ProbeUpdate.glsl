//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "../Common/Math.glsl"

#define CACHE_SIZE 64

#if defined(DEPTHPROBE_UPDATE)
    #define NUM_THREADS_X 16
    #define NUM_THREADS_Y 16
    #define TEXTURE_WIDTH ddgi.depthTextureWidth
    #define TEXTURE_HEIGHT ddgi.depthTextureHeight
    #define PROBE_SIDE_LENGTH ddgi.depthProbeSideLength
#else
    #define NUM_THREADS_X 8
    #define NUM_THREADS_Y 8
    #define TEXTURE_WIDTH ddgi.irradianceTextureWidth
    #define TEXTURE_HEIGHT ddgi.irradianceTextureHeight
    #define PROBE_SIDE_LENGTH ddgi.irradianceProbeSideLength
#endif

layout(local_size_x = NUM_THREADS_X, local_size_y = NUM_THREADS_Y, local_size_z = 1) in;


layout(set = 0, binding = 0, rgba16f)   uniform image2D uOutIrradiance;
layout(set = 0, binding = 1, rg16f)     uniform image2D uOutDepth;

layout(set = 1, binding = 0) uniform sampler2D uInputIrradiance;
layout(set = 1, binding = 1) uniform sampler2D uInputDepth;
layout(set = 1, binding = 2, scalar) uniform DDGIUBO
{
    DDGIUniform ddgi; 
};

layout(set = 2, binding = 0) uniform sampler2D uInputRadiance;
layout(set = 2, binding = 1) uniform sampler2D uInputDirectionDepth;

layout(push_constant) uniform PushConstants
{
    uint firstFrame;
}pushConsts;


shared vec4 gRayDirectionDepth[CACHE_SIZE];

#if !defined(DEPTHPROBE_UPDATE) 
shared vec3 gRayHitRadiance[CACHE_SIZE];
#endif

const float FLT_EPS = 0.00000001;

void loadCache(int relativeProbeId, uint offset, uint numRays)
{
    if (gl_LocalInvocationIndex < numRays)
    {
        ivec2 C = ivec2(offset + uint(gl_LocalInvocationIndex), relativeProbeId);

        gRayDirectionDepth[gl_LocalInvocationIndex] = texelFetch(uInputDirectionDepth, C, 0);
    #if !defined(DEPTHPROBE_UPDATE) 
        gRayHitRadiance[gl_LocalInvocationIndex] = texelFetch(uInputRadiance, C, 0).xyz;
    #endif 
    }
}

void gatherRays(ivec2 currentCoord, uint numRays, inout vec3 result, inout float totalWeight)
{
    for (int r = 0; r < numRays; ++r)
    {
        vec4 rayDirectionDepth = gRayDirectionDepth[r];

        vec3 rayDirection = rayDirectionDepth.xyz;

#if defined(DEPTHPROBE_UPDATE)            
        float rayProbeDistance = min(ddgi.maxDistance, rayDirectionDepth.w - 0.01f);
            
        if (rayProbeDistance == -1.0f)
            rayProbeDistance = ddgi.maxDistance;
#else        
        vec3  rayHitRadiance   = gRayHitRadiance[r];
#endif

        vec3 texelDirection = octDecode(normalizedOctCoord(currentCoord, PROBE_SIDE_LENGTH));

        float weight = 0.0f;

#if defined(DEPTHPROBE_UPDATE)  
        weight = pow(max(0.0, dot(texelDirection, rayDirection)), ddgi.sharpness);
#else
        weight = max(0.0, dot(texelDirection, rayDirection));
#endif

        if (weight >= FLT_EPS)
        {
#if defined(DEPTHPROBE_UPDATE) 
            result += vec3(rayProbeDistance * weight, square(rayProbeDistance) * weight, 0.0);
#else
            result += vec3(rayHitRadiance * weight);
#endif                
            totalWeight += weight;
        }
    }
}

void main()
{
    const ivec2 currentCoord = ivec2(gl_GlobalInvocationID.xy) + (ivec2(gl_WorkGroupID.xy) * ivec2(2)) + ivec2(2);

    const int relativeProbeId = getProbeId(currentCoord, TEXTURE_WIDTH, PROBE_SIDE_LENGTH);
    
    vec3  result       = vec3(0.0f);
    float totalWeight = 0.0f;

    uint remainingRays = ddgi.raysPerProbe;
    uint offset = 0;

    while (remainingRays > 0)
    {
        uint numRays = min(CACHE_SIZE, remainingRays);
        
        loadCache(relativeProbeId, offset, numRays);

        barrier();

        gatherRays(currentCoord, numRays, result, totalWeight);

        barrier();

        remainingRays -= numRays;
        offset += numRays;
    }
    
    if (totalWeight > FLT_EPS)
        result *= 1.f / ( 2 * totalWeight );

#if defined(DEPTHPROBE_UPDATE)
    vec3 prevResult;
    prevResult.rg = texelFetch(uInputDepth, currentCoord, 0).rg;
#else
    vec3 prevResult = texelFetch(uInputIrradiance, currentCoord, 0).rgb;
    result.rgb = pow(result.rgb, vec3(1.0f / ddgi.ddgiGamma));
#endif
            
    if (pushConsts.firstFrame == 0)            
        result = mix(result, prevResult, ddgi.hysteresis);

#if defined(DEPTHPROBE_UPDATE)
    imageStore(uOutDepth, currentCoord, vec4(result, 1.0));
#else
    imageStore(uOutIrradiance, currentCoord, vec4(result, 1.0));
#endif
}

// ------------------------------------------------------------------