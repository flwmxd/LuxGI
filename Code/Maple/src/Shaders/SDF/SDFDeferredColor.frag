//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : require

#include "../Common/Math.glsl"
#include "../Common/GBufferSampler.glsl"


layout(location = 0) in vec4 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec4 fragNormal;
layout(location = 4) in vec3 fragTangent;

layout(set = 1, binding = 0) uniform sampler2D uAlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D uMetallicMap;
layout(set = 1, binding = 2) uniform sampler2D uRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D uNormalMap;
layout(set = 1, binding = 4) uniform sampler2D uAOMap;
layout(set = 1, binding = 5) uniform sampler2D uEmissiveMap;
layout(set = 1, binding = 6) uniform UniformMaterialData
{
	MaterialProperties materialProperties;
}ubo;

//bind to framebuffer
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPBR;
layout(location = 3) out vec4 outEmissive;


void main()
{
	vec4 texColor = getAlbedo(ubo.materialProperties,uAlbedoMap, fragTexCoord) * fragColor;
	if(texColor.w < 0.01)
		discard;

	float metallic = 0.0;
	float roughness = 0.0;
    float ao = getAO(ubo.materialProperties,uAOMap,fragTexCoord);

	if(ubo.materialProperties.workflow == PBR_WORKFLOW_SEPARATE_TEXTURES)
	{
		metallic  = getMetallic(ubo.materialProperties,uMetallicMap, fragTexCoord).x;
		roughness = getRoughness(ubo.materialProperties,uRoughnessMap, fragTexCoord);
	}
	else if( ubo.materialProperties.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)
	{
		vec3 tex = texture(uMetallicMap, fragTexCoord).rgb;
		//ao  	  = tex.r;
		metallic  = tex.b;
 		roughness = tex.g;
	}
	else if( ubo.materialProperties.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)
	{
		vec3 tex = texture(uMetallicMap, fragTexCoord).rgb;
		metallic = tex.b;
		roughness = tex.g * ubo.materialProperties.roughnessColor.r;
	}

	vec3 emissive   = getEmissive(ubo.materialProperties, uEmissiveMap,fragTexCoord);

	vec3 normal 	= getNormalFromMap(ubo.materialProperties, uNormalMap, fragTexCoord, fragPosition.xyz, fragNormal.xyz);
    outColor    	= vec4(gammaCorrectTextureRGB(texColor), fragPosition.x);
	outNormal   	= vec4(directionToOctohedral(normal), fragPosition.y, fragPosition.z);
	outPBR      	= vec4(metallic, roughness, ao, 1.f);
	outEmissive 	= vec4(emissive, 1);
}