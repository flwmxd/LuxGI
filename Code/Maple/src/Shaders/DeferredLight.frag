//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : require

#include "Common/Light.glsl"
#include "Common/Math.glsl"
#include "Common/GBufferSampler.glsl"

#define ShadowTypeShadowMap 0
#define ShadowTypeTraceShadowCone 1
#define ShadowTypeRaytraceShadow 2
#define ShadowTypeSDFShadow 3

struct GBufferMaterial
{
	vec4 albedo;
	vec3 metallic;
	float roughness;
	vec3 normal;
	float ao;
	float ssao;
	vec3 view;
	float normalDotView;
};

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0)  uniform sampler2D uColorSampler;
layout(set = 0, binding = 1)  uniform sampler2D uNormalSampler;
layout(set = 0, binding = 2)  uniform sampler2D uDepthSampler;
layout(set = 0, binding = 3)  uniform sampler2D uPBRSampler;
layout(set = 0, binding = 4)  uniform sampler2DArray uShadowMap;
layout(set = 0, binding = 5)  uniform sampler2D uIrradianceSH;
layout(set = 0, binding = 6)  uniform samplerCube uPrefilterMap;
layout(set = 0, binding = 7)  uniform sampler2D uPreintegratedFG;
layout(set = 0, binding = 8) uniform sampler2D uIndirectLight;
layout(set = 0, binding = 9) uniform sampler2D uShadowMapRaytrace;
layout(set = 0, binding = 10) uniform sampler2D uReflection;
layout(set = 0, binding = 11) uniform UniformBufferLight
{
	Light lights[MAX_LIGHTS];// 16 * 32
	mat4 shadowTransform[MAX_SHADOWMAPS]; // 16 * 32 * 4

	mat4 viewMatrix;
	mat4 lightView;
	mat4 biasMat;
	mat4 viewProjInv;
	mat4 viewProj;

	vec4 cameraPosition;
	vec4 splitDepths[MAX_SHADOWMAPS];

	float initialBias;
	uint shadowMapSize;
	int lightCount;
	int shadowCount;

	int mode;
	int cubeMapMipLevels;
	int ssaoEnable;
	int enableLPV;

	int enableShadow;
	int shadowMethod;
	int enableIBL;
	int enableDDGI;

	int enableVXGI;
	int enableFog;
	vec2 farNear;
} ubo;
layout(set = 0, binding = 14) uniform sampler2D uEmissiveSampler;

float textureProj(vec4 shadowCoord, vec2 offset, int cascadeIndex)
{
	float shadow = 1.0;
	float ambient = 0.00;
	
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 && shadowCoord.w > 0)
	{
		float dist = texture(uShadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (dist < (shadowCoord.z - ubo.initialBias))
		{
			shadow = ambient;//dist;
		}
	}
	return shadow;
}

float PCFShadow(vec4 sc, int cascadeIndex)
{
	ivec2 texDim = textureSize(uShadowMap, 0).xy;
	float scale = 0.75;
	
	vec2 dx = scale * 1.0 / texDim;
	
	float shadowFactor = 0.0;
	int count = 0;
	float range = 1.0;
	
	for (float x = -range; x <= range; x += 1.0) 
	{
		for (float y = -range; y <= range; y += 1.0) 
		{
			shadowFactor += textureProj(sc, vec2(dx.x * x, dx.y * y), cascadeIndex);
			count++;
		}
	}
	return shadowFactor / count;
}


float getShadowBias(vec3 lightDirection, vec3 normal, int shadowIndex)
{
	float minBias = ubo.initialBias;
	float bias = max(minBias * (1.0 - dot(normal, lightDirection)), minBias);
	return bias;
}

vec3 lighting(vec3 F0, vec3 wsPos, GBufferMaterial material,vec2 fragTexCoord)
{
	vec3 result = vec3(0.0);
	
	if( ubo.lightCount == 0)
	{
		return material.albedo.rgb;
	}
	
	float value = 1.0;
	
	for(int i = 0; i < ubo.lightCount; i++)
	{
		Light light = ubo.lights[i];

		float intensity = pow(light.intensity,1.4) + 0.1;

		vec3 lightColor = light.color.xyz * intensity;
		vec3 indirect = vec3(0,0,0);
		vec3 lightDir = vec3(0,0,0);
		if(light.type == LIGHT_POINT)
		{
		    // Vector to light
			vec3 L = light.position.xyz - wsPos;
			// Distance from light to fragment position
			float dist = length(L);
			
			// Light to fragment
			L = normalize(L);
			
			// Attenuation
			float atten = light.radius / (pow(dist, 2.0) + 1.0);
			
			value = atten;
			lightDir = L;
		}
		else if (light.type == LIGHT_SPOT)
		{
			vec3 L = light.position.xyz - wsPos;
			float cutoffAngle   = 1.0f - light.angle;      
			float dist          = length(L);
			L = normalize(L);
			float theta         = dot(L.xyz, light.direction.xyz);
			float epsilon       = cutoffAngle - cutoffAngle * 0.9f;
			float attenuation 	= ((theta - cutoffAngle) / epsilon); // atteunate when approaching the outer cone
			attenuation         *= light.radius / (pow(dist, 2.0) + 1.0);//saturate(1.0f - dist / light.range);
			value = clamp(attenuation, 0.0, 1.0);
			lightDir = L;
		}
		else
		{
			lightDir = light.direction.xyz * -1;
			if(ubo.enableShadow == 1.0 && ubo.shadowMethod == ShadowTypeShadowMap)
			{
				int cascadeIndex = calculateCascadeIndex(
					ubo.viewMatrix,wsPos,ubo.shadowCount,ubo.splitDepths
				);
				vec4 shadowCoord = (ubo.biasMat * ubo.shadowTransform[cascadeIndex]) * vec4(wsPos, 1.0);
				shadowCoord = shadowCoord * ( 1.0 / shadowCoord.w);
				value = PCFShadow(shadowCoord , cascadeIndex);
			}
		}

		vec3 Li = lightDir.xyz;
		vec3 Lradiance = lightColor;
		vec3 Lh = normalize(Li + material.view);
		float cosLi = max(0.0, dot(material.normal, Li));
		vec3 directShading = BRDF(material.albedo.xyz, material.normal, material.roughness, material.metallic.x, material.view, Lh, Li)* Lradiance * cosLi;

		result += directShading;
	}

	if (ubo.shadowMethod == ShadowTypeRaytraceShadow || ubo.shadowMethod == ShadowTypeSDFShadow)
	{
		value *= texture(uShadowMapRaytrace,fragTexCoord).r;
	}
	return result * value;
}

vec3 evaluateSH9Irradiance(in vec3 direction)
{
	const float CosineA0 = PI;
	const float CosineA1 = (2.0 * PI) / 3.0;
	const float CosineA2 = PI * 0.25;

    SH9 basis;

    projectToSh9(direction, basis);

    basis.c[0] *= CosineA0;
    basis.c[1] *= CosineA1;
    basis.c[2] *= CosineA1;
    basis.c[3] *= CosineA1;
    basis.c[4] *= CosineA2;
    basis.c[5] *= CosineA2;
    basis.c[6] *= CosineA2;
    basis.c[7] *= CosineA2;
    basis.c[8] *= CosineA2;

    vec3 color = vec3(0.0);

    for (int i = 0; i < 9; i++)
        color += texelFetch(uIrradianceSH, ivec2(i, 0), 0).rgb * basis.c[i];

    color.x = max(0.0, color.x);
    color.y = max(0.0, color.y);
    color.z = max(0.0, color.z);
    return color / PI;
}

vec3 IBL(vec3 F0, vec3 Lr, GBufferMaterial material)
{
	vec3 F = fresnelSchlickRoughness(material.normalDotView, F0, material.roughness);
	vec3 kd = (1.0 - F) * (1.0 - material.metallic.r);

	float level = float(ubo.cubeMapMipLevels);
	vec3 irradiance = evaluateSH9Irradiance(material.normal);
	vec3 diffuseIBL = irradiance * material.albedo.rgb;

	vec3 specularIrradiance = textureLod(uPrefilterMap, Lr, material.roughness * level).rgb;
	vec2 specularBRDF = texture(uPreintegratedFG, vec2(material.normalDotView, material.roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);
	
	return kd * diffuseIBL + specularIBL;
}

vec3 gammaCorrectTextureRGB(vec3 texCol)
{
	return vec3(pow(texCol.rgb, vec3(GAMMA)));
}

float attentuate( vec3 lightData, float dist )
{
	float att =  1.0 / ( lightData.x + lightData.y*dist + lightData.z*dist*dist );
	float damping = 1.0;
	return max(att * damping, 0.0);
}


float getSpecularOcclusion(float NoV, float roughnessSq, float ao)
{
    return clamp(pow(NoV + ao, roughnessSq) - 1 + ao, 0, 1);
}

vec3 indirectLight(in GBufferMaterial material, vec3 F0, vec2 fragTexCoord)
{
	vec3 indirectShading = vec3(0);
	vec3 specular = vec3(0);

	if(ubo.enableDDGI == 1)
	{
		indirectShading  = texture(uIndirectLight,fragTexCoord).rgb;
		vec3 diffuseBRDF = (material.albedo.xyz - material.albedo.xyz * material.metallic) / PI;
		indirectShading = diffuseBRDF * indirectShading;

		vec4 reflection = textureLod(uReflection, fragTexCoord, 0.0f);
		vec3 specularIrradiance = reflection.rgb;//* reflection.a;
		vec2 specularBRDF = texture(uPreintegratedFG, vec2(material.normalDotView, material.roughness)).rg;
		specular = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);//todo...

		float roughnessSq = material.roughness * material.roughness;
		float specularOcclusion = getSpecularOcclusion(material.normalDotView, roughnessSq, material.ao);
		specular *= specularOcclusion;
	}
	return indirectShading + specular;
}


void main()
{
	vec4 albedo = texture(uColorSampler, fragTexCoord);
	if (albedo.a < 0.1) {
		discard;
	}
	vec4 pbr 		= texture(uPBRSampler,	fragTexCoord);
	float depth 	= texture(uDepthSampler, fragTexCoord).r;
#ifdef GL_NDC
	depth = depth * 2.0 - 1.0;
#endif
	vec3 worldPos 	= worldPositionFromDepth(fragTexCoord,depth,ubo.viewProjInv);
	vec4 normalTex	= texture(uNormalSampler, fragTexCoord);
	vec4 emissive = texture(uEmissiveSampler,fragTexCoord);

	GBufferMaterial material;
    material.albedo			= albedo;
    material.metallic		= vec3(pbr.r);
    material.roughness		= pbr.g;
    material.normal			= normalize(octohedralToDirection(normalTex.xy));
	material.ao				= emissive.a;
	material.ssao			= 1;
	material.view 			= normalize(ubo.cameraPosition.xyz - worldPos);
	material.normalDotView  = clamp(dot(material.normal, material.view), 0.0, 1.0);

	vec3 F0 = mix(Fdielectric, material.albedo.rgb, material.metallic.r);
	vec3 iblContribution = vec3(0,0,0);
	
	if(ubo.enableIBL == 1)
	{
		vec3 Lr =  reflect(material.view,material.normal); 
		iblContribution = IBL(F0, Lr, material);
	}

	vec3 lightContribution = lighting(F0, worldPos, material,fragTexCoord);
	vec3 indirect = indirectLight(material,F0, fragTexCoord);
	vec3 finalColor = (lightContribution + iblContribution + indirect) * material.ao * material.ssao;

	outColor = vec4(finalColor + emissive.rgb, 1.0);
}
