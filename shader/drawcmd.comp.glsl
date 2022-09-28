#version 460

#define CULL 1

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

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


    drawCommands[di].indexCount = draws[di].indexCount;
    drawCommands[di].instanceCount = visible ? 1 : 0;
    drawCommands[di].firstIndex = draws[di].indexOffset;
    drawCommands[di].vertexOffset = draws[di].vertexOffset;
    drawCommands[di].firstInstance = 0;
    drawCommands[di].taskCount = visible ? ((draws[di].meshletCount + 31) / 32) : 0;
    drawCommands[di].firstTask = draws[di].meshletOffset / 32;
}