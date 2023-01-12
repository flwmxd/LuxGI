//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450

#extension GL_EXT_debug_printf : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : require

#include "../Common/Math.glsl"

layout (location = 0) in vec3 sampleDirection;

layout (set = 0,binding = 0) uniform samplerCube uCubeMapSampler;


layout (location = 0) out vec4 outColor;

void main()
{		
    // The sample direction equals the hemisphere's orientation 
    vec3 normal = normalize(sampleDirection);
  
    vec3 irradiance = vec3(0.0);

	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = cross(up, normal);
	up = cross(normal, right);

	const float sampleDelta = 0.025;
	float nrSamples = 0.0;
	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
	    for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
	    {
	        // Spherical to cartesian (in tangent space)
	        vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
	        // Tangent space to world
	        vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 

	        irradiance += textureLod(uCubeMapSampler, sampleVec,0).rgb * cos(theta) * sin(theta);
	        ++nrSamples;
	    }
	}
	irradiance = PI * irradiance * (1.0 / nrSamples);

    outColor = vec4(irradiance, 1.0);
}
