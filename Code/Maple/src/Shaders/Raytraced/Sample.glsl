#ifndef SAMPLE_GLSL
#define SAMPLE_GLSL

const float PBR_WORKFLOW_SEPARATE_TEXTURES = 0.0f;
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;

layout (set = 4, binding = 0) uniform sampler2D uSamplers[];

void getAlbedo(in Material material, inout SurfaceMaterial p)
{
    if (material.textureIndices0.x >= 0)
        p.albedo =  (1.0 - material.usingValue0.x) * material.albedo + material.usingValue0.x *  textureLod(uSamplers[nonuniformEXT(material.textureIndices0.x)], p.vertex.texCoord.xy, 0.0);
    else
        p.albedo =  material.albedo;
    p.albedo = gammaCorrectTexture(p.albedo);
}

vec3 getNormalFromMap(in Vertex vertex, uint normalMapIndex)
{
	vec3 tangentNormal = texture(uSamplers[nonuniformEXT(normalMapIndex)] , vertex.texCoord).xyz * 2.0 - 1.0;
	
	vec3 N = normalize(vertex.normal);
	vec3 T = normalize(vertex.tangent);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	
	return normalize(TBN * tangentNormal);
}

void getNormal(in Material material, inout SurfaceMaterial p)
{
    if (material.textureIndices1.x >= 0)
        p.normal = getNormalFromMap(p.vertex, material.textureIndices1.x);
    else
        p.normal = p.vertex.normal.xyz;
}

void getRoughness(in Material material, inout SurfaceMaterial p)
{
    if (material.textureIndices0.z < 0)
    {
        p.roughness = material.roughness.r;
    }
    else
    {
        if(material.usingValue1.w == PBR_WORKFLOW_SEPARATE_TEXTURES)
        {
            p.roughness = (1 - material.usingValue0.z ) * material.roughness.r +  ( material.usingValue0.z ) * textureLod(uSamplers[nonuniformEXT(material.textureIndices0.z)], p.vertex.texCoord.xy, 0.0).r;
        }
        else if(material.usingValue1.w == PBR_WORKFLOW_METALLIC_ROUGHNESS)
        {
            p.roughness = (1 - material.usingValue0.z ) * material.roughness.r +  ( material.usingValue0.z ) * textureLod(uSamplers[nonuniformEXT(material.textureIndices0.w)], p.vertex.texCoord.xy, 0.0).g;
        }
        else if(material.usingValue1.w == PBR_WORKFLOW_SPECULAR_GLOSINESS)
        {
            p.roughness = (1 - material.usingValue0.z ) * material.roughness.r + ( material.usingValue0.z ) * textureLod(uSamplers[nonuniformEXT(material.textureIndices0.w)], p.vertex.texCoord.xy, 0.0).g;
        }
        else
        {
            p.roughness = material.roughness.r;
        }
    }
}

void getMetallic(in Material material, inout SurfaceMaterial p)
{
    if (material.textureIndices0.w < 0)
    {
        p.metallic = material.metallic.r;
    }
    else
    {
        if(material.usingValue1.w == PBR_WORKFLOW_SEPARATE_TEXTURES)
        {
            p.metallic = ( 1 - material.usingValue0.y ) * material.metallic.r + ( material.usingValue0.y ) *textureLod(uSamplers[nonuniformEXT(material.textureIndices0.w)], p.vertex.texCoord.xy, 0.0).r;
        }
        else if(material.usingValue1.w == PBR_WORKFLOW_METALLIC_ROUGHNESS)
        {
            p.metallic = ( 1 - material.usingValue0.y ) * material.metallic.r + ( material.usingValue0.y ) *textureLod(uSamplers[nonuniformEXT(material.textureIndices0.w)], p.vertex.texCoord.xy, 0.0).b;
        }
        else if(material.usingValue1.w == PBR_WORKFLOW_SPECULAR_GLOSINESS)
        {
            p.metallic = ( 1 - material.usingValue0.y ) * material.metallic.r + ( material.usingValue0.y ) *textureLod(uSamplers[nonuniformEXT(material.textureIndices0.w)], p.vertex.texCoord.xy, 0.0).b;
        }
        else
        {
             p.metallic = material.metallic.r;
        }
    }
}

void getEmissive(in Material material, inout SurfaceMaterial p)
{
    if (material.textureIndices1.x == -1)
        p.emissive = material.emissive;
    else
    {
        p.emissive.rgb =  ( 1 - material.usingValue1.z ) * material.emissive.rgb +  material.usingValue1.z * textureLod(uSamplers[nonuniformEXT(material.textureIndices1.x)], p.vertex.texCoord.xy, 0.0).rgb;
        p.emissive.a = material.emissive.a;
    }
}


#endif