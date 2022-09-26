#version 460

#define CULL 1

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_NV_mesh_shader: require

#extension GL_GOOGLE_include_directive: require

#extension GL_KHR_shader_subgroup_ballot: require

#include "mesh_struct.h"

layout(local_size_x = 32, local_size_y = 1, local_size_x = 1) in;

layout(push_constant) uniform block
{
    MeshDraw meshDraw;
};

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

void main() {
    uint mgi = gl_WorkGroupID.x;
    uint ti = gl_LocalInvocationID.x;
    uint mi = mgi * 32 + ti;

#if CULL
    bool accept = !coneCull(meshlets[mi].cone, vec3(0, 0, 1));
    uvec4 ballot = subgroupBallot(accept);
    uint index = subgroupBallotExclusiveBitCount(ballot);

    if (accept)
    {
        meshletIndices[index] = mi;
    }

    uint count = subgroupBallotBitCount(ballot);

    if (ti == 0)
    {
        gl_TaskCountNV = count;
    }
#else
    meshletIndices[ti] = mi;
    if (ti == 0)
    {
        gl_TaskCountNV = 32;
    }
#endif
}