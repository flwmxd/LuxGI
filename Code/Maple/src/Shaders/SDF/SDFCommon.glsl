//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#ifndef SDF_COMMON
#define SDF_COMMON

#define GLOBAL_SDF_WORLD_SIZE 60000.0f
#define GLOBAL_SDF_RASTERIZE_CHUNK_SIZE 32
#define GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN 4

#include "ObjectRasterizeData.glsl"
#include "GlobalSDFData.glsl"
#include "GlobalSDFHit.glsl"
#include "GlobalSDFTrace.glsl"


float combineDistanceToSDF(float sdf, float distanceToSDF)
{
	// Negative distinace inside the SDF
	if (sdf <= 0 && distanceToSDF <= 0) return sdf;
	// Worst-case scenario with triangle edge (C^2 = A^2 + B^2)

    /* but it is not accuratly
    *          ______ 
    *         /_____/ |
    *   A     |B    | |
    *         |   C | |
    *          ------ /
    *                       ->
    *  the distance should |AC|  but we lost the c coord, only have |BC|
    *  using (AC^2 = AB^2 + BC^2)
    */
	float maxSDF = max(sdf, 0);
	return sqrt(maxSDF * maxSDF + distanceToSDF * distanceToSDF); 
}

float distanceToModelSDF(float minDistance, in ObjectRasterizeData modelData, sampler3D modelSDFTex, vec3 worldPos)
{
	// Compute SDF volume UVs and distance in world-space to the volume bounds
	vec3 volumePos = (modelData.worldToVolume * vec4(worldPos, 1)).xyz;
	vec3 volumeUV = volumePos * modelData.volumeToUVWMul + modelData.volumeToUVWAdd;

	vec3 volumePosClamped = clamp(volumePos, -modelData.volumeLocalBoundsExtent, modelData.volumeLocalBoundsExtent);
	vec4 worldPosClamped = modelData.volumeToWorld * vec4(volumePosClamped,1);

	float distanceToVolume = distance(worldPos, worldPosClamped.xyz);
	if (distanceToVolume < 0.01f)
	    distanceToVolume = length(volumePos - volumePosClamped);
    distanceToVolume = max(distanceToVolume, 0);

	if (minDistance <= distanceToVolume) return distanceToVolume;

	float volumeDistance = (textureLod(modelSDFTex, volumeUV, modelData.mipOffset).r * 2.f - 1.) * modelData.decodeMul;

	float result = combineDistanceToSDF(volumeDistance, distanceToVolume);
	if (distanceToVolume > 0)
	{
		result = max(distanceToVolume, result);
	}
	return result;
}

void getGlobalSDFCascadeUV(in GlobalSDFData data, uint cascade, vec3 worldPosition, out float cascadeMaxDistance, out vec3 cascadeUV, out vec3 textureUV)
{
    vec4 cascadePosDistance = data.cascadePosDistance[cascade];
    vec3 posInCascade = worldPosition - cascadePosDistance.xyz;
    cascadeMaxDistance = cascadePosDistance.w * 2;
    cascadeUV = clamp( posInCascade / cascadeMaxDistance + 0.5f, vec3(0) , vec3(1) );
    textureUV = vec3((cascade + cascadeUV.x) / float(data.cascadesCount), cascadeUV.y, cascadeUV.z);
}

bool isHit(in GlobalSDFHit hit)
{
    return hit.hitTime >= 0.0f;
}

vec3 getHitPosition(in GlobalSDFHit hit, in GlobalSDFTrace trace)
{
    return trace.worldPosition + trace.worldDirection * hit.hitTime;
}

//(x - closest, y - furthest).
vec2 lineHitAABB(vec3 lineStart, vec3 lineEnd, vec3 boxMin, vec3 boxMax)
{
    vec3 invDirection = 1.0f / (lineEnd - lineStart);
    vec3 enterIntersection = (boxMin - lineStart) * invDirection;
    vec3 exitIntersection = (boxMax - lineStart) * invDirection;
    vec3 minIntersections = min(enterIntersection, exitIntersection);
    vec3 maxIntersections = max(enterIntersection, exitIntersection);
    vec2 intersections;
    intersections.x = max(minIntersections.x, max(minIntersections.y, minIntersections.z));
    intersections.y = min(maxIntersections.x, min(maxIntersections.y, maxIntersections.z));
    return clamp(intersections,0,1);
}


GlobalSDFHit tracyGlobalSDF(in GlobalSDFData data, 
                            sampler3D tex, 
                            sampler3D mip, 
                            in GlobalSDFTrace trace, 
                            float cascadeTraceStartBias)
{
    GlobalSDFHit hit;
    hit.stepsCount = 0;
    hit.hitTime = -1.0f;
    hit.hitNormal = vec3(0);
    hit.hitCascade = 0;
    hit.hitSDF = 0.0;

    float traceMaxDistance = min(trace.maxDistance, data.cascadePosDistance[ data.cascadesCount - 1 ].w * 2);
    
    float chunkSizeDistance     = float(GLOBAL_SDF_RASTERIZE_CHUNK_SIZE) /  data.resolution;  // Size of the chunk in SDF distance (0-1)
    float chunkMarginDistance   = float(GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN) / data.resolution; // Size of the chunk margin in SDF distance (0-1)
    
    float nextIntersectionStart = 0.0f;
    vec3  traceEndPosition = trace.worldPosition + trace.worldDirection * traceMaxDistance;

    for (uint cascade = 0; cascade < data.cascadesCount && hit.hitTime < 0.0f; cascade++)
    {
        vec4  cascadePosDistance = data.cascadePosDistance[cascade];
        float voxelSize          = data.cascadeVoxelSize[cascade];
        float voxelHalf          = voxelSize * 0.5f;
        vec3  worldPosition      = trace.worldPosition + trace.worldDirection * (voxelSize * cascadeTraceStartBias);

        vec2 intersections  = lineHitAABB(worldPosition, traceEndPosition, cascadePosDistance.xyz - cascadePosDistance.www, cascadePosDistance.xyz + cascadePosDistance.www);
        intersections.xy    *= traceMaxDistance;
        intersections.x     = max(intersections.x, nextIntersectionStart);

        float stepTime      = intersections.x;
        if (intersections.x >= intersections.y)// closest is greater than furthest
        {
            stepTime = intersections.y;
        }
        else
        {
            // Skip the current cascade, tracing on the next cascade
            nextIntersectionStart = intersections.y;
        }

        //Raymarching
        uint step = 0;
        for (; step < 250 && stepTime < intersections.y; step++)
        {
            vec3 stepPosition = worldPosition + trace.worldDirection * stepTime;

            float cascadeMaxDistance;
            vec3 cascadeUV, textureUV;
            getGlobalSDFCascadeUV(data, cascade, stepPosition, cascadeMaxDistance, cascadeUV, textureUV);
            float stepDistance = texture(mip,textureUV).r;
            if (stepDistance < chunkSizeDistance)
            {
                float stepDistanceTex = texture(tex,textureUV).r;
                if (stepDistanceTex < chunkMarginDistance * 2)
                {
                    stepDistance = stepDistanceTex;
                }
            }
            else
            {
                stepDistance = chunkSizeDistance;
            }

            stepDistance *= cascadeMaxDistance;

            float minSurfaceThickness = voxelHalf * clamp(stepTime / voxelSize, 0.0, 1.0);
            if (stepDistance < minSurfaceThickness)
            {
                hit.hitTime = max(stepTime + stepDistance - minSurfaceThickness, 0.0f);
                hit.hitCascade = cascade;
                hit.hitSDF = stepDistance;
                if (trace.needsHitNormal)
                {
                    // Calculate hit normal from SDF gradient
                    //dU/dX, dU/dY
                    float texelOffset = 1.0f / data.resolution;
                    float xp = texture(tex, vec3(textureUV.x + texelOffset, textureUV.y, textureUV.z)).r;
                    float xn = texture(tex, vec3(textureUV.x - texelOffset, textureUV.y, textureUV.z)).r;
                    float yp = texture(tex, vec3(textureUV.x, textureUV.y + texelOffset, textureUV.z)).r;
                    float yn = texture(tex, vec3(textureUV.x, textureUV.y - texelOffset, textureUV.z)).r;
                    float zp = texture(tex, vec3(textureUV.x, textureUV.y, textureUV.z + texelOffset)).r;
                    float zn = texture(tex, vec3(textureUV.x, textureUV.y, textureUV.z - texelOffset)).r;
                    hit.hitNormal = normalize(vec3(xp - xn, yp - yn, zp - zn));
                }
                break;
            }

            // Move 
            stepTime += max(stepDistance * trace.stepScale, voxelSize);
        }
        hit.stepsCount += step;
    }
    return hit;
}

float getGlobalSurfaceAtlasThreshold(in GlobalSDFData data, in GlobalSDFHit hit)
{
    return data.cascadeVoxelSize[hit.hitCascade] * 1.05f;
}

#endif
