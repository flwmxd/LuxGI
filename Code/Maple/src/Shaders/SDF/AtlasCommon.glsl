#ifndef ATLAS_COMMON
#define ATLAS_COMMON

// Amount of chunks (in each direction) to split atlas draw distance for objects culling
#define GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION 40 
#define GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD 0.05f

struct GlobalSurfaceAtlasData
{
    vec3  cameraPos;
    float chunkSize;
    uint  culledObjectsCapacity;
    uint  resolution;
    uint  objectsCount;
    uint  padding;
};

struct ObjectBuffer
{
    vec4   objectBounds;
    uint   tileOffset[6];
    ivec2  padding;
    mat4   transform;
    vec4   extends;
};

struct TileBuffer
{
    vec4 extends;
    mat4 transform; 
    vec4 objectBounds;
};

bool boxIntersectsSphere(vec3 boxMin, vec3 boxMax, vec3 sphereCenter, float sphereRadius)
{
    const vec3 clampedCenter = clamp(sphereCenter, boxMin, boxMax);
    return distance(sphereCenter, clampedCenter) <= sphereRadius;
}

uint flattenId(ivec3 chunkCoord)
{
    return  chunkCoord.z * 
            GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * 
            GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION + 
            chunkCoord.y * 
            GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION + 
            chunkCoord.x;
}

vec3 sampleGlobalSurfaceAtlasTex(sampler2D atlas, vec2 atlasUV, vec4 bilinearWeights)
{
    vec4 sampleX = textureGather(atlas, atlasUV, 0);//
    vec4 sampleY = textureGather(atlas, atlasUV, 1);//
    vec4 sampleZ = textureGather(atlas, atlasUV, 2);//
    return vec3(dot(sampleX, bilinearWeights), dot(sampleY, bilinearWeights), dot(sampleZ, bilinearWeights));
}

vec4 sampleGlobalSurfaceAtlasTile(in GlobalSurfaceAtlasData data, in ObjectBuffer object, in TileBuffer tile, 
                                    sampler2D depth, sampler2D atlas, vec3 worldPosition, float normalWeight, float surfaceThreshold)
{

    vec3 tilePosition =(tile.transform * vec4(worldPosition, 1)).xyz;
    float tileDepth = tilePosition.z / tile.objectBounds.z;
    vec2 tileUV = clamp( ( tilePosition.xy / tile.objectBounds.xy ) + 0.5 , vec2(0), vec2(1));

    vec2 atlasUV = tileUV * tile.extends.zw + tile.extends.xy;
    // Calculate bilinear weights
    vec2 bilinearWeightsUV = fract(atlasUV * data.resolution + 0.5f);
    vec4 bilinearWeights;
    bilinearWeights.x = (1.0 - bilinearWeightsUV.x) * (bilinearWeightsUV.y);
    bilinearWeights.y = (bilinearWeightsUV.x) * (bilinearWeightsUV.y);
    bilinearWeights.z = (bilinearWeightsUV.x) * (1 - bilinearWeightsUV.y);
    bilinearWeights.w = (1 - bilinearWeightsUV.x) * (1 - bilinearWeightsUV.y);

    vec4 tileZ = textureGather(depth, atlasUV, 0);
    float depthThreshold = 2.0f * surfaceThreshold / tile.objectBounds.z;
    vec4 depthVisibility = vec4(1.0f);
    for (uint i = 0; i < 4; i++)
    {
        depthVisibility[i] = 1.0f - clamp((abs(tileDepth - tileZ[i]) - depthThreshold) / (0.5f * depthThreshold), 0, 1);
        if (tileZ[i] >= 1.0f)
            depthVisibility[i] = 0.0f;
    }

    float sampleWeight = dot(depthVisibility, bilinearWeights);

    sampleWeight *= normalWeight;

    if (sampleWeight <= 0.0f)
        return vec4(0);        

    bilinearWeights *= depthVisibility;
    // Sample atlas texture
    vec3 sampleColor = sampleGlobalSurfaceAtlasTex(atlas, atlasUV, bilinearWeights);

    return vec4(sampleColor.rgb * sampleWeight, sampleWeight);
}


vec4 sampleGlobalSurfaceAtlasTile(in GlobalSurfaceAtlasData data, in ObjectBuffer object, in TileBuffer tile, 
                                    sampler2D depth, sampler2D atlas, vec3 worldPosition, vec4 worldNormal, float surfaceThreshold)
{ 

    vec3 normal = (tile.transform * worldNormal).xyz;
    normal = normalize(normal);
    float normalWeight = clamp(dot(vec3(0, 0, 1), normal),0,1);
    normalWeight = (normalWeight - GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD) / (1.0f - GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD);
    if (normalWeight <= 0.0f )
        return vec4(0);
    
    return sampleGlobalSurfaceAtlasTile(data,object,tile,depth,atlas,worldPosition, normalWeight, surfaceThreshold);    
}

//in GlobalSurfaceAtlasData data, uint chunks[] , uint cullObjects[] ,ObjectBuffer data[],  TileBuffer tiles[], sampler2D depth, sampler2D atlas, vec3 worldPosition, vec3 worldNormal
#define sampleGlobalSurfaceAtlas(result, data, chunks, cullObjects, objects, tiles, depth, atlas, worldPosition, worldNormal, surfaceThreshold, debug) \
{                                                                                                                                                   \
    result = vec4(0, 0, 0, 0);                                                                                                                      \
    ivec3 chunkCoord = clamp( ivec3(floor( (worldPosition /*- data.cameraPos*/) / data.chunkSize + (GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * 0.5f)) ), ivec3(0), ivec3(GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION - 1)); \
    uint chunkAddress = flattenId(chunkCoord);          \
    uint objectsStart = chunks[chunkAddress];           \
    if (objectsStart != 0)                              \
    {                                                   \
        uint objectsCount = cullObjects[objectsStart];  \
        if (objectsCount <= data.objectsCount)          \
        {                                               \
            objectsStart++;                             \
            vec4 fallbackColor = vec4(0);               \
            for (uint objectIndex = 0; objectIndex < objectsCount; objectIndex++)             \
            {                                                                                 \
                uint objectAddress = cullObjects[objectsStart++];                             \
                ObjectBuffer object = objects[objectAddress];                                 \
                if (distance(object.objectBounds.xyz, worldPosition) > object.objectBounds.w) \
                    continue;                                                                 \
                mat4 worldToLocal = inverse(object.transform);                                \
                vec3 localPosition = (worldToLocal * vec4(worldPosition, 1)).xyz;             \
                vec3 localExtents = object.extends.xyz + surfaceThreshold;                    \
                if (any(greaterThan(abs(localPosition) , localExtents)))                      \
                    continue;                                                                 \
                vec3 normal = normalize(mat3(worldToLocal) * worldNormal);                    \
                for(uint i = 0;i<6;i++)                                                       \
                {                                                                             \
                    uint tileOffset = object.tileOffset[i];                                   \
                    if(tileOffset != 0)                                                       \
                    {                                                                         \
                        TileBuffer tile = tiles[tileOffset];                                  \
                        result += sampleGlobalSurfaceAtlasTile(data, object, tile, depth, atlas, localPosition, vec4(normal,1.0), surfaceThreshold);\
                    }                                                                         \
                }                                                                             \
            }                                                                                 \
            if(debug){                                                                        \
                if (result.a < 0.05f)                                                         \
                    result = vec4(1, 0, 1, 1);                                                \
            }                                                                                 \
            result.rgb /= max(result.a, 0.0001f);                                             \
        }                                                                                     \
    }                                                                                         \
}

#endif