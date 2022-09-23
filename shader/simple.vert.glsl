#version 460

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_struct.h"

layout(push_constant) uniform block
{
    MeshDraw meshDraw;
};

layout(binding = 0) buffer readonly Vertices
{
    Vertex vertices[];
};

// layout(location = 0) in vec3 inPosition;
// layout(location = 1) in vec3 inNormal;
// layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;

void main() {
    Vertex v = vertices[gl_VertexIndex];

    vec3 inPosition = vec3(v.vx, v.vy, v.vz);
    vec3 inNormal = vec3(int(v.nx), int(v.ny), int(v.nz)) / 127.0 - 1.0;
    vec2 inTexCoord  = vec2(v.tu, v.tv);

    gl_Position = vec4((inPosition * vec3(meshDraw.scale, 1) + vec3(meshDraw.offset, 0))
         * vec3(2, 2, -0.5) + vec3(-1, -1, 0.5), 1.0);
    fragColor = normalize(inNormal) * 0.5 + 0.5;
}