//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : require

#include "Common/Math.glsl"
#include "Common/GBufferSampler.glsl"


layout(location = 0) in vec4 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec4 fragNormal;
layout(location = 4) in vec3 fragTangent;
layout(location = 5) in vec4 fragProjPosition;
layout(location = 6) in vec4 fragOldProjPosition;
layout(location = 7) in flat uint meshId;

layout(set = 1, binding = 0) uniform sampler2D uAlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D uMetallicMap;
layout(set = 1, binding = 2) uniform sampler2D uRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D uNormalMap;
layout(set = 1, binding = 4) uniform sampler2D uAOMap;
layout(set = 1, binding = 5) uniform sampler2D uEmissiveMap;
layout(set = 1, binding = 6) uniform UniformMaterialData
{
	MaterialProperties materialProperties;
} ubo1;


layout(set = 2,binding = 0) uniform UBO
{
	mat4 view;
	float nearPlane;
	float farPlane;
	float padding;
	float padding2;
}ubo;

//bind to framebuffer
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPBR;
layout(location = 3) out vec4 outEmissive;


float computeCurvature(float depth)
{
    vec3 dx = dFdx(fragNormal.xyz);
    vec3 dy = dFdy(fragNormal.xyz);

    float x = dot(dx, dx);
    float y = dot(dy, dy);

    return pow(max(x, y), 0.5f);
}

void main()
{
	vec4 texColor = getAlbedo(ubo1.materialProperties,uAlbedoMap, fragTexCoord) * fragColor;
	if(texColor.w < 0.01)
		discard;

	float metallic = 0.0;
	float roughness = 0.0;
	float ao = getAO(ubo1.materialProperties,uAOMap,fragTexCoord);

	if(ubo1.materialProperties.workflow == PBR_WORKFLOW_SEPARATE_TEXTURES)
	{
		metallic  = getMetallic(ubo1.materialProperties,uMetallicMap, fragTexCoord).x;
		roughness = getRoughness(ubo1.materialProperties,uRoughnessMap, fragTexCoord);
	}
	else if( ubo1.materialProperties.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)
	{
		vec3 tex = texture(uMetallicMap, fragTexCoord).rgb;
		//ao  	  = tex.r;
		metallic  = tex.b;
 		roughness = tex.g;
	}
	else if( ubo1.materialProperties.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)
	{
		vec3 tex = texture(uMetallicMap, fragTexCoord).rgb;
		metallic = tex.b;
		roughness = tex.g * ubo1.materialProperties.roughnessColor.r;
	}

	vec2 a = (fragProjPosition.xy / fragProjPosition.w) * 0.5 + 0.5;
    vec2 b = (fragOldProjPosition.xy / fragOldProjPosition.w) * 0.5 + 0.5;

	vec3 emissive   = getEmissive(ubo1.materialProperties, uEmissiveMap,fragTexCoord);
	float linearZ 	= gl_FragCoord.z /  gl_FragCoord.w;
    float curvature = computeCurvature(linearZ);
	vec3 normal 	= getNormalFromMap(ubo1.materialProperties, uNormalMap, fragTexCoord, fragPosition.xyz, fragNormal.xyz);

    outColor    	= vec4(gammaCorrectTextureRGB(texColor), float(meshId));
	outNormal   	= vec4(directionToOctohedral(normal),  b - a);
	outPBR      	= vec4(metallic, roughness, curvature, linearZ);
	outEmissive 	= vec4(emissive, ao);
}