#version 460

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_NV_mesh_shader: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_struct.h"

layout(local_size_x = 32, local_size_y = 1, local_size_x = 1) in;

out taskNV block 
{
    uint meshletIndices[32];
};

layout(binding = 1) buffer readonly Meshlets
{
    Meshlet meshlets[];
};

bool coneCull(vec4 cone, vec3 view)
{
    return dot(cone.xyz, view) > cone.w;
}

shared uint meshletCount;

void main() {
    uint mgi = gl_WorkGroupID.x;
    uint ti = gl_LocalInvocationID.x;
    uint mi = mgi * 32 + ti;

    meshletCount = 0;

    memoryBarrierShared();

    if (!(coneCull(meshlets[mi].cone, vec3(0, 0, 1))))
    {
        uint index = atomicAdd(meshletCount, 1);
        meshletIndices[index] = mi;
    }

    memoryBarrierShared();

    if (ti == 0)
    {
        gl_TaskCountNV = meshletCount;
    }
}