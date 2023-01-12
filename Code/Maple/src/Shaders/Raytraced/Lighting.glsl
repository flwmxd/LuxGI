#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#include "../Common/Light.glsl"
#include "BRDF.glsl"
#include "Common.glsl"
#include "RayQuery.glsl"

layout (set = 0, binding = 2, std430) readonly buffer LightBuffer 
{
    Light data[];
} Lights;

vec3 sampleLight(in SurfaceMaterial p, in Light light, out vec3 Wi, in accelerationStructureEXT uTopLevelAS)
{
    vec3 Li = vec3(0.0f);
    if (light.type == LIGHT_DIRECTIONAL)
    {
        vec3 lightDir = -light.direction.xyz;
        Wi = lightDir;
        Li = light.color.xyz * (pow(light.intensity,1.4) + 0.1);
        float v = queryDistance( p.vertex.position + p.normal * 0.1f, Wi , 10000.f , uTopLevelAS);
        Li *= v;
    } 
    else if(light.type == LIGHT_POINT)
    {
        // Vector to light
        vec3 lightDir = light.position.xyz - p.vertex.position.xyz;
        // Distance from light to fragment position
        float dist = length(lightDir);
        // Light to fragment
        lightDir = normalize(lightDir);
        // Attenuation
        float atten = light.radius / (pow(dist, 2.0) + 1.0);
        Wi = lightDir;
        Li = atten * light.color.xyz * (pow(light.intensity,1.4) + 0.1);
    }
    else if(light.type == LIGHT_SPOT)
    {
        vec3 L = light.position.xyz -  p.vertex.position.xyz;
        float cutoffAngle   = 1.0f - light.angle;      
        float dist          = length(L);
        L = normalize(L);
        float theta         = dot(L.xyz, light.direction.xyz);
        float epsilon       = cutoffAngle - cutoffAngle * 0.9f;
        float attenuation 	= ((theta - cutoffAngle) / epsilon); // atteunate when approaching the outer cone
        attenuation         *= light.radius / (pow(dist, 2.0) + 1.0);//saturate(1.0f - dist / light.range);
    
        float value = clamp(attenuation, 0.0, 1.0);

        Li = light.color.xyz * (pow(light.intensity,1.4) + 0.1) * value;
        Wi = L;
    }
    return Li;
}

vec3 directLighting(in SurfaceMaterial p, int numLights, in accelerationStructureEXT uTopLevelAS)
{
    vec3 L = vec3(0.0f);
       
    for(int lightIdx = 0;lightIdx<numLights;lightIdx++)
    {
        const Light light = Lights.data[lightIdx];

        if(light.type == LIGHT_ENV)
            continue;
        vec3 view = -gl_WorldRayDirectionEXT;
        vec3 lightDir = vec3(0.0f);
        vec3 halfV = vec3(0.0f);
        vec3 Li = sampleLight(p, light, lightDir,uTopLevelAS);
        halfV = normalize(lightDir + view);
        vec3 brdf = BRDF(p, view, halfV, lightDir);
        float cosTheta = clamp(dot(p.normal, lightDir), 0.0, 1.0);
        L += brdf * cosTheta * Li;
    }
    return L;
}

vec3 indirectLighting(in SurfaceMaterial p, sampler2D uIrradiance, sampler2D uDepth, in DDGIUniform ddgi)
{
    vec3 Wo = -gl_WorldRayDirectionEXT;
    vec3 diffuseColor = (p.albedo.rgb - p.albedo.rgb * p.metallic) / PI;
    return diffuseColor * sampleIrradiance(ddgi, p.vertex.position, p.normal, Wo, uIrradiance, uDepth);
}

#endif