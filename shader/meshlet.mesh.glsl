#version 460

#define DEBUG 1

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_NV_mesh_shader: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_struct.h"

layout(local_size_x = 32, local_size_y = 1, local_size_x = 1) in;
layout(triangles, max_vertices = 64, max_primitives = MESHLETTRICOUNT) out;

layout(binding = 0) buffer readonly Vertices
{
    Vertex vertices[];
};

layout(binding = 1) buffer readonly Meshlets
{
    Meshlet meshlets[];
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
    uint mi = gl_WorkGroupID.x;
    uint ti = gl_LocalInvocationID.x;

    uint vertexCount = uint(meshlets[mi].vertexCount);
    uint triangleCount = uint(meshlets[mi].triangleCount);
    uint indexCount = triangleCount * 3;

    #if DEBUG
        uint mhash = hash(mi);
        vec3 mcolor = vec3(float((mhash & 255)), float((mhash >> 8) & 255), float((mhash >> 16) & 255)) / 255.f;
    #endif

    for (uint i = ti; i < vertexCount; i += 32)
    {
        uint vi = meshlets[mi].vertices[i];
        Vertex v = vertices[vi];

        vec3 inPosition = vec3(v.vx, v.vy, v.vz);
        vec3 inNormal = vec3(int(v.nx), int(v.ny), int(v.nz)) / 127.0 - 1.0;
        vec2 inTexCoord  = vec2(v.tu, v.tv);

        gl_MeshVerticesNV[i].gl_Position = vec4(inPosition + vec3(0, 0, 0.5), 1.0);
        fragColor[i] = normalize(inNormal) * 0.5 + 0.5;

    #if DEBUG
        fragColor[i] = mcolor;
    #endif
    }

    // for (uint i = ti; i < triangleCount; i += 32)
    // {
    //     uint vi0 = meshlets[mi].vertices[uint(meshlets[mi].indices[3*i + 0])];
    //     uint vi1 = meshlets[mi].vertices[uint(meshlets[mi].indices[3*i + 1])];
    //     uint vi2 = meshlets[mi].vertices[uint(meshlets[mi].indices[3*i + 2])];

    //     vec3 inPosition0 = vec3(vertices[vi0].vx, vertices[vi0].vy, vertices[vi0].vz);
    //     vec3 inPosition1 = vec3(vertices[vi1].vx, vertices[vi1].vy, vertices[vi1].vz);
    //     vec3 inPosition2 = vec3(vertices[vi2].vx, vertices[vi2].vy, vertices[vi2].vz);
        
    //     // Code below this works fine, proving that we are in the same warp (SIMD) and all vertices are already filled

    //     // uint vi0 = uint(meshlets[mi].indices[3*i + 0]);
    //     // uint vi1 = uint(meshlets[mi].indices[3*i + 1]);
    //     // uint vi2 = uint(meshlets[mi].indices[3*i + 2]);

    //     // vec3 inPosition0 = gl_MeshVerticesNV[vi0].gl_Position.xyz;
    //     // vec3 inPosition1 = gl_MeshVerticesNV[vi1].gl_Position.xyz;
    //     // vec3 inPosition2 = gl_MeshVerticesNV[vi2].gl_Position.xyz;

    //     vec3 normal = normalize(cross(inPosition1 - inPosition0, inPosition2 - inPosition0));

    //     triangleNormal[i] = normal;
    // }

    //uint indexChunkCount = (indexCount + 3) / 4;

    for (uint i = ti; i < indexCount; i += 32)
    {
        gl_PrimitiveIndicesNV[i] = uint(meshlets[mi].indices[i]);

        //writePackedPrimitiveIndices4x8NV(i * 4, meshlets[mi].indicesPacked[i]);
    }

    if (ti == 0)
    {
        gl_PrimitiveCountNV = uint(meshlets[mi].triangleCount);
    }
}