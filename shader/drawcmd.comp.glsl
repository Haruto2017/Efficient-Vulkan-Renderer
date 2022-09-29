#version 460

#define CULL 1

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require
#extension GL_KHR_shader_subgroup_ballot: require

#include "mesh_struct.h"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block
{
    vec4 frustum[6];
};

layout(binding = 0) buffer readonly Draws
{
    MeshDraw draws[];
};

layout(binding = 1) buffer writeonly DrawCommands
{
    MeshDrawCommand drawCommands[];
};

layout(binding = 2) buffer DrawCommandCount
{
    uint drawCommandCount;
};

void main()
{
    uint gi = gl_WorkGroupID.x;
    uint ti = gl_LocalInvocationID.x;
    uint di = gi * 32 + ti;

    vec3 center = rotate(draws[di].center, draws[di].rotation) * draws[di].scale + draws[di].position;
    float radius = draws[di].radius * draws[di].scale;

    bool visible = true;

    #if CULL
    for (int i = 0; i < 6; ++ i)
    {
        visible = visible && (dot(vec4(center, 1), frustum[i]) > -radius);
    }
    #endif

    uvec4 ballot = subgroupBallot(visible);
    uint count = subgroupBallotBitCount(ballot);

    if (count == 0)
    {
        return;
    }

    uint dcgi = 0;

    if (ti == 0)
    {
        dcgi = atomicAdd(drawCommandCount, count);
    }
    
    uint index = subgroupBallotExclusiveBitCount(ballot);
    uint dci = subgroupBroadcastFirst(dcgi) + index;

    if (visible)
    {
        drawCommands[dci].drawId = di;
        drawCommands[dci].indexCount = draws[di].indexCount;
        drawCommands[dci].instanceCount = visible ? 1 : 0;
        drawCommands[dci].firstIndex = draws[di].indexOffset;
        drawCommands[dci].vertexOffset = draws[di].vertexOffset;
        drawCommands[dci].firstInstance = 0;
        drawCommands[dci].taskCount = visible ? ((draws[di].meshletCount + 31) / 32) : 0;
        drawCommands[dci].firstTask = draws[di].meshletOffset / 32;
    }
}