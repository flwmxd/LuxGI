//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#ifndef REFLECTION_COMMON
#define REFLECTION_COMMON

#define MIRROR_REFLECTIONS_ROUGHNESS_THRESHOLD 0.05f
#define DDGI_REFLECTIONS_ROUGHNESS_THRESHOLD 0.45f

struct ReflectionPayload
{
    vec3 color;
    float rayLength;
};

/**
* get luminance, 0.299, 0.587, 0.114 is the Grayscale
*/
float luminance(vec3 rgb)
{
    return max(dot(rgb, vec3(0.299, 0.587, 0.114)), 0.0001);
}

#endif