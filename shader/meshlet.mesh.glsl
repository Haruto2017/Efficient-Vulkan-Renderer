#version 460

#define DEBUG 0

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_NV_mesh_shader: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_struct.h"

layout(local_size_x = 32, local_size_y = 1, local_size_x = 1) in;
layout(triangles, max_vertices = 64, max_primitives = MESHLETTRICOUNT) out;

layout(push_constant) uniform block
{
    MeshDraw meshDraw;
};

layout(binding = 0) buffer readonly Vertices
{
    Vertex vertices[];
};

layout(binding = 1) buffer readonly Meshlets
{
    Meshlet meshlets[];
};

layout(binding = 2) buffer readonly MeshletData
{
    uint meshletData[];
};

in taskNV block 
{
    uint meshletIndices[32];
};

layout(location = 0) out vec3 fragColor[];

// layout(location = 1)
// perprimitiveNV out vec3 triangleNormal[];

uint hash(uint a)
{
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return a;
}

void main() {
    uint mi = meshletIndices[gl_WorkGroupID.x];
    uint ti = gl_LocalInvocationID.x;

    uint vertexCount = uint(meshlets[mi].vertexCount);
    uint triangleCount = uint(meshlets[mi].triangleCount);
    uint indexCount = triangleCount * 3;

    uint dataOffset = meshlets[mi].dataOffset;
    uint vertexOffset = dataOffset;
    uint indexOffset = dataOffset + vertexCount;

    #if DEBUG
        uint mhash = hash(mi);
        vec3 mcolor = vec3(float((mhash & 255)), float((mhash >> 8) & 255), float((mhash >> 16) & 255)) / 255.f;
    #endif

    for (uint i = ti; i < vertexCount; i += 32)
    {
        uint vi = meshletData[vertexOffset + i];
        Vertex v = vertices[vi];

        vec3 inPosition = vec3(v.vx, v.vy, v.vz);
        vec3 inNormal = vec3(int(v.nx), int(v.ny), int(v.nz)) / 127.0 - 1.0;
        vec2 inTexCoord  = vec2(v.tu, v.tv);

        gl_MeshVerticesNV[i].gl_Position = meshDraw.projection * vec4(rotate(inPosition, meshDraw.rotation) * meshDraw.scale + meshDraw.position, 1.0);
        fragColor[i] = normalize(inNormal) * 0.5 + 0.5;

    #if DEBUG
        fragColor[i] = mcolor;
    #endif
    }

    uint indexGroupCount = (indexCount + 3) / 4;

    for (uint i = ti; i < indexGroupCount; i += 32)
    {
        writePackedPrimitiveIndices4x8NV(i * 4, meshletData[indexOffset + i]);
    }

    if (ti == 0)
    {
        gl_PrimitiveCountNV = uint(meshlets[mi].triangleCount);
    }
}