#version 450

struct Vertex
{
    float vx, vy, vz;
    float nx, ny, nz;
    float tu, tv;
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
    vec3 inNormal = vec3(v.nx, v.ny, v.nz);
    vec2 inTexCoord  = vec2(v.tu, v.tv);

    gl_Position = vec4(inPosition + vec3(0, 0, 0.5), 1.0);
    fragColor = normalize(inNormal) * 0.5 + 0.5;
}