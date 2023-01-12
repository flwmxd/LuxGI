//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#ifndef RAY_QUERY_GLSL
#define RAY_QUERY_GLSL

float queryVisibility(vec3 worldPos, vec3 direction, float tmax, uint rayFlags, in accelerationStructureEXT uTopLevelAS)
{
    float t_min = 0.01f;
 
    rayQueryEXT rayQuery;

    rayQueryInitializeEXT(rayQuery,
                          uTopLevelAS,
                          rayFlags,
                          0xFF,
                          worldPos,
                          t_min,
                          direction,
                          tmax);

    while (rayQueryProceedEXT(rayQuery)) {}

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT)
        return 0.0f;

    return 1.0f;
}

float queryDistance(vec3 worldPos, vec3 direction, float tmax, in accelerationStructureEXT uTopLevelAS)
{
    float t_min     = 0.01f;
    uint  rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT;

    rayQueryEXT rayQuery;

    rayQueryInitializeEXT(rayQuery,
                          uTopLevelAS,
                          rayFlags,
                          0xFF,
                          worldPos,
                          t_min,
                          direction,
                          tmax);

    // Start traversal
    while (rayQueryProceedEXT(rayQuery)) {}

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT)
        return rayQueryGetIntersectionTEXT(rayQuery, true) < tmax ? 0.0f : 1.0f;

    return 1.0f;
}

#endif