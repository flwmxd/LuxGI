#ifndef GBUFFER_SAMPLER
#define GBUFFER_SAMPLER
#include "../Raytraced/BRDF.glsl"
#define GAMMA 2.2

const float PBR_WORKFLOW_SEPARATE_TEXTURES = 0.0f;
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;

struct MaterialProperties
{
    vec4  albedoColor;
    vec4  roughnessColor;
    vec4  metallicColor;
    vec4  emissiveColor;
    float usingAlbedoMap;
    float usingMetallicMap;
    float usingRoughnessMap;
    float usingNormalMap;
    float usingAOMap;
    float usingEmissiveMap;
    float workflow;
    float padding;
};

vec3 gammaCorrectTextureRGB(vec4 samp)
{
	return vec3(pow(samp.rgb, vec3(GAMMA)));
}

vec4 getAlbedo(in MaterialProperties materialProperties, sampler2D uAlbedoMap, vec2 fragTexCoord )
{
	return (1.0 - materialProperties.usingAlbedoMap) * materialProperties.albedoColor + materialProperties.usingAlbedoMap * texture(uAlbedoMap, fragTexCoord);
}

vec3 getMetallic(in MaterialProperties materialProperties, sampler2D uMetallicMap, vec2 fragTexCoord )
{
	return (1.0 - materialProperties.usingMetallicMap) * materialProperties.metallicColor.rgb + materialProperties.usingMetallicMap * texture(uMetallicMap, fragTexCoord).rgb;
}

float getRoughness(in MaterialProperties materialProperties, sampler2D uRoughnessMap, vec2 fragTexCoord )
{
	return (1.0 - materialProperties.usingRoughnessMap) *  materialProperties.roughnessColor.r + materialProperties.usingRoughnessMap * texture(uRoughnessMap, fragTexCoord).r;
}

float getAO(in MaterialProperties materialProperties, sampler2D uAOMap, vec2 fragTexCoord )
{
	return (1.0 - materialProperties.usingAOMap) + materialProperties.usingAOMap * gammaCorrectTextureRGB(texture(uAOMap, fragTexCoord)).r;
}

vec3 getEmissive(in MaterialProperties materialProperties, sampler2D uEmissiveMap, vec2 fragTexCoord )
{
	vec3 color = (1.0 - materialProperties.usingEmissiveMap) * materialProperties.emissiveColor.rgb + materialProperties.usingEmissiveMap * gammaCorrectTextureRGB(texture(uEmissiveMap, fragTexCoord));
	color *= materialProperties.emissiveColor.a;
	return color;
}

vec3 getNormalFromMap(in MaterialProperties materialProperties, sampler2D uNormalMap, vec2 fragTexCoord, vec3 fragPosition, vec3 fragNormal)
{
	if (materialProperties.usingNormalMap < 0.1)
		return normalize(fragNormal.xyz);
	
	vec3 tangentNormal = texture(uNormalMap, fragTexCoord).xyz * 2.0 - 1.0;
	
	vec3 Q1 = dFdx(fragPosition.xyz);
	vec3 Q2 = dFdy(fragPosition.xyz);
	vec2 st1 = dFdx(fragTexCoord);
	vec2 st2 = dFdy(fragTexCoord);
	vec3 T = normalize(Q1*st2.t - Q2*st1.t);
	
	vec3 N = normalize(fragNormal.xyz);
	// T = normalize(fragTangent);
	vec3 B = -normalize(cross(N, T));

	mat3 TBN = mat3(T, B, N);
	
	return normalize(TBN * tangentNormal);
}
#endif