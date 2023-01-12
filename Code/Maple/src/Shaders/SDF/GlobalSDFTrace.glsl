#ifndef GLOBAL_SDF_TRACE
#define GLOBAL_SDF_TRACE

struct GlobalSDFTrace
{
    vec3 worldPosition;
    float minDistance;
    vec3 worldDirection;
    float maxDistance;
    float stepScale;
    bool needsHitNormal;
};

#endif