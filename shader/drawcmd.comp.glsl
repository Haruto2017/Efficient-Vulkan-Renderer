#version 460

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_struct.h"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) buffer readonly Draws
{
    MeshDraw draws[];
};

layout(binding = 1) buffer writeonly DrawCommands
{
    MeshDrawCommand drawCommands[];
};

void main()
{
    uint gi = gl_WorkGroupID.x;
    uint ti = gl_LocalInvocationID.x;
    uint di = gi * 32 + ti;

    drawCommands[di].indexCount = draws[di].indexCount;
    drawCommands[di].instanceCount = 1;
    drawCommands[di].firstIndex = draws[di].indexOffset;
    drawCommands[di].vertexOffset = draws[di].vertexOffset;
    drawCommands[di].firstInstance = 0;
    drawCommands[di].taskCount = (draws[di].meshletCount + 31) / 32;
    drawCommands[di].firstTask = 0;
}