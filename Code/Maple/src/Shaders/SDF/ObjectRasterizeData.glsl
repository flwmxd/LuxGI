//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#ifndef OBJECT_RASTERIZE_DATA
#define OBJECT_RASTERIZE_DATA

struct ObjectRasterizeData
{
	mat4  worldToVolume;
	mat4  volumeToWorld;
	vec3  volumeToUVWMul;
	float mipOffset;
    vec3  volumeToUVWAdd;
	float decodeMul;
	vec3  volumeLocalBoundsExtent;
	float decodeAdd;
};

#endif


