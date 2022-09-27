#version 460

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types: require

#extension GL_ARB_shader_draw_parameters : require

#extension GL_GOOGLE_include_directive: require

#include "mesh_struct.h"

layout(push_constant) uniform block
{
    Globals globals;
};

layout(binding = 0) buffer readonly Draws
{
    MeshDraw draws[];
};

layout(binding = 1) buffer readonly Vertices
{
    Vertex vertices[];
};

// layout(location = 0) in vec3 inPosition;
// layout(location = 1) in vec3 inNormal;
// layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;

void main() {
    Vertex v = vertices[gl_VertexIndex];

    MeshDraw meshDraw = draws[gl_DrawIDARB];

    vec3 inPosition = vec3(v.vx, v.vy, v.vz);
    vec3 inNormal = vec3(int(v.nx), int(v.ny), int(v.nz)) / 127.0 - 1.0;
    vec2 inTexCoord  = vec2(v.tu, v.tv);

    gl_Position = globals.projection * vec4(rotate(inPosition, meshDraw.rotation) * meshDraw.scale + meshDraw.position, 1.0);
    fragColor = normalize(inNormal) * 0.5 + 0.5;
}