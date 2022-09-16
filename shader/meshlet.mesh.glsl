#version 460

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_NV_mesh_shader: require

layout(local_size_x = 1, local_size_y = 1, local_size_x = 1) in;
layout(triangles, max_vertices = 64, max_primitives = 42) out;

struct Vertex
{
    float16_t vx, vy, vz;
    uint8_t nx, ny, nz, nw;
    float16_t tu, tv;
};

layout(binding = 0) buffer readonly Vertices
{
    Vertex vertices[];
};

struct Meshlet
{
	uint vertices[64];
	uint8_t indices[126]; // up to 42 triangles
	uint8_t indexCount; 
	uint8_t vertexCount;
};

layout(binding = 1) buffer readonly Meshlets
{
    Meshlet meshlets[];
};

layout(location = 0) out vec3 fragColor[];

void main() {
    uint mi = gl_WorkGroupID.x;
    for (uint i = 0; i < uint(meshlets[mi].vertexCount); ++ i)
    {
        uint vi = meshlets[mi].vertices[i];
        Vertex v = vertices[vi];

        vec3 inPosition = vec3(v.vx, v.vy, v.vz);
        vec3 inNormal = vec3(int(v.nx), int(v.ny), int(v.nz)) / 127.0 - 1.0;
        vec2 inTexCoord  = vec2(v.tu, v.tv);

        gl_MeshVerticesNV[i].gl_Position = vec4(inPosition + vec3(0, 0, 0.5), 1.0);
        fragColor[i] = normalize(inNormal) * 0.5 + 0.5;
    }

    gl_PrimitiveCountNV = uint(meshlets[mi].indexCount) / 3;

    for (uint i = 0; i < uint(meshlets[mi].indexCount); ++ i)
    {
        gl_PrimitiveIndicesNV[i] = uint(meshlets[mi].indices[i]);
    }
}