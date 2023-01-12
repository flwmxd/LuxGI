//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "ObjectRasterizeData.glsl"
#include "SDFCommon.glsl"

#define GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT 28
#define LOCAL_SIZE_X 8
#define LOCAL_SIZE_Y 8
#define LOCAL_SIZE_Z 8

layout(local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;


layout(set = 0, binding = 0) uniform ModelsRasterizeData
{
	vec3  cascadeCoordToPosMul;
	float maxDistance;
	vec3  cascadeCoordToPosAdd;
	int   cascadeResolution;
	int   cascadeIndex;
	vec3  padding;
}ubo;

layout(set = 0, binding = 1, std430) readonly buffer SDFObjectData{
    ObjectRasterizeData data[];
};

layout(set = 0, binding = 2,r16f) uniform image3D uGlobalSDF;

layout(set = 0, binding = 3) uniform sampler3D uMeshSDF[];

layout(push_constant) uniform PushConsts
{
	ivec3  chunkCoord;
	int    objectsCount;
	uint  objects[GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT];
} pushConsts;


void main()
{ 
    ivec3 uvw = ivec3(gl_GlobalInvocationID.xyz);
	ivec3 voxelCoord = pushConsts.chunkCoord + uvw;//from every chunk..
	vec3 voxelWorldPos = voxelCoord * ubo.cascadeCoordToPosMul + ubo.cascadeCoordToPosAdd;
	voxelCoord.x += ubo.cascadeIndex *  ubo.cascadeResolution;//
	float minDistance =  ubo.maxDistance;

#ifdef READ_DISTANCE
	minDistance *= imageLoad(uGlobalSDF,voxelCoord).r;
#endif
 
	for (uint i = 0; i < pushConsts.objectsCount; i++)
	{
		uint objId = pushConsts.objects[i];
		ObjectRasterizeData objectData = data[objId];
		float objectDistance = distanceToModelSDF(minDistance, objectData, uMeshSDF[objId], voxelWorldPos);
		minDistance = min(minDistance, objectDistance);
	}
 	vec4 v = vec4( clamp(minDistance / ubo.maxDistance, -1, 1));
	imageStore(uGlobalSDF,voxelCoord, v);
}
