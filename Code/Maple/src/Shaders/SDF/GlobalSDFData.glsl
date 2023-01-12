#ifndef GLOBAL_SDF_DATA
#define GLOBAL_SDF_DATA

struct GlobalSDFData
{
    vec4 cascadePosDistance[4];//center -> xyz / distance -> w
    vec4 cascadeVoxelSize;
    uint cascadesCount;
    float resolution;
    float nearPlane;
    float farPlane;
};

#endif