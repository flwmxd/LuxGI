#ifndef GLOBAL_SDF_HIT
#define GLOBAL_SDF_HIT

struct GlobalSDFHit
{
    vec3 hitNormal;
    float hitTime;
    uint hitCascade;
    uint stepsCount;
    float hitSDF;
};


#endif
