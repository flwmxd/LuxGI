//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#ifndef LIGHT_GLSL
#define LIGHT_GLSL

/**
 * DirectionalLight = 0,
 * SpotLight = 1,
 * PointLight = 2,
 */

#define LIGHT_DIRECTIONAL 0
#define LIGHT_SPOT 1.0
#define LIGHT_POINT 2.0
#define LIGHT_ENV 3.0
#define LIGHT_AREA 4.0

struct Light
{
	vec4 color;
	vec4 position;
	vec4 direction;
	float intensity;
	float radius;
	float type;
	float angle;
};

#define MAX_LIGHTS 32

#define MAX_SHADOWMAPS 4

int calculateCascadeIndex(mat4 viewMatrix, vec3 wsPos, int shadowCount, vec4 splitDepths[MAX_SHADOWMAPS])
{
	int cascadeIndex = 0;
	vec4 viewPos = viewMatrix * vec4(wsPos, 1.0) ;

	for(int i = 0; i < shadowCount - 1; ++i)
	{
		if(viewPos.z < splitDepths[i].x)
		{
			cascadeIndex = i + 1;
		}
	}
	return cascadeIndex;
}

struct SH9
{
    float c[9];
};


struct SH9Color
{
    vec3 c[9];
};

void projectToSh9(in vec3 dir, inout SH9 sh)
{
    // Band 0
    sh.c[0] = 0.282095;

    // Band 1
    sh.c[1] = -0.488603 * dir.y;
    sh.c[2] = 0.488603 * dir.z;
    sh.c[3] = -0.488603 * dir.x;

    // Band 2
    sh.c[4] = 1.092548 * dir.x * dir.y;
    sh.c[5] = -1.092548 * dir.y * dir.z;
    sh.c[6] = 0.315392 * (3.0 * dir.z * dir.z - 1.0);
    sh.c[7] = -1.092548 * dir.x * dir.z;
    sh.c[8] = 0.546274 * (dir.x * dir.x - dir.y * dir.y);
}


#endif