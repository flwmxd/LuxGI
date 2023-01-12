#ifndef TRIANGLE_GLSL
#define TRIANGLE_GLSL

layout (set = 1, binding = 0, scalar) readonly buffer VertexBuffer 
{
    Vertex data[];
} Vertices[];

layout (set = 2, binding = 0) readonly buffer IndexBuffer 
{
    uint data[];
} Indices[];
///////////////////////////////////////////////////////////////

layout (set = 3, binding = 0) readonly buffer SubmeshInfoBuffer 
{
    uvec2 data[];
} SubmeshInfo[];

HitInfo getHitInfo()
{
    uvec2 primitiveOffsetMatIdx = SubmeshInfo[nonuniformEXT(gl_InstanceCustomIndexEXT)].data[gl_GeometryIndexEXT];
    HitInfo hitInfo;
    hitInfo.materialIndex   = primitiveOffsetMatIdx.y;
    hitInfo.primitiveOffset = primitiveOffsetMatIdx.x;
    hitInfo.primitiveId     = gl_PrimitiveID;
    return hitInfo;
}

Vertex getVertex(uint meshIdx, uint vertexIdx)
{
    return Vertices[nonuniformEXT(meshIdx)].data[vertexIdx];
}

Triangle getTriangle(in Transform transform, in HitInfo hitInfo)
{
    Triangle tri;

    uint primitiveId =  hitInfo.primitiveId + hitInfo.primitiveOffset;

    uvec3 idx = uvec3(Indices[nonuniformEXT(transform.meshIdx)].data[3 * primitiveId], 
                      Indices[nonuniformEXT(transform.meshIdx)].data[3 * primitiveId + 1],
                      Indices[nonuniformEXT(transform.meshIdx)].data[3 * primitiveId + 2]);

    tri.v0 = getVertex(transform.meshIdx, idx.x);
    tri.v1 = getVertex(transform.meshIdx, idx.y);
    tri.v2 = getVertex(transform.meshIdx, idx.z);
    return tri;
}

void transformVertex(in Transform transform, inout Vertex v)
{
    mat4 modelMat = transform.modelMatrix;
    mat3 normalMat = mat3(transform.normalMatrix);

    vec4 newPos     = modelMat * vec4(v.position,1);
    v.position      = newPos.xyz;
    v.normal.xyz    = normalMat * v.normal.xyz;
    v.tangent.xyz   = normalMat * v.tangent.xyz;
}

#endif