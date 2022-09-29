#version 460

#define CULL 1

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_NV_mesh_shader: require

#extension GL_ARB_shader_draw_parameters : require

#extension GL_GOOGLE_include_directive: require

#extension GL_KHR_shader_subgroup_ballot: require

#include "mesh_struct.h"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

out taskNV block 
{
    uint meshletIndices[32];
};

layout(binding = 0) buffer readonly DrawCommands
{
    MeshDrawCommand drawCommands[];
};

layout(binding = 1) buffer readonly Draws
{
    MeshDraw draws[];
};

layout(binding = 2) buffer readonly Meshlets
{
    Meshlet meshlets[];
};

bool coneCull(vec3 center, float radius, vec3 cone_axis, float cone_cutoff , vec3 camera_position)
{
    return dot(center - camera_position, cone_axis) >= cone_cutoff * length(center - camera_position) + radius;
}

void main() {
    uint mgi = gl_WorkGroupID.x;
    uint ti = gl_LocalInvocationID.x;
    uint mi = mgi * 32 + ti;

    MeshDraw meshDraw = draws[drawCommands[gl_DrawIDARB].drawId];

#if CULL
    vec3 center = rotate(meshlets[mi].center, meshDraw.rotation) * meshDraw.scale + meshDraw.position;
    float radius = meshlets[mi].radius * meshDraw.scale;
    vec3 cone_axis = rotate(vec3(int(meshlets[mi].cone_axis[0]) / 127.0, int(meshlets[mi].cone_axis[1]) / 127.0, int(meshlets[mi].cone_axis[2]) / 127.0), meshDraw.rotation);
    float cone_cutoff = int(meshlets[mi].cone_cutoff) / 127.0;

    bool accept = !coneCull(center, radius, cone_axis, cone_cutoff, vec3(0, 0, 0));

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