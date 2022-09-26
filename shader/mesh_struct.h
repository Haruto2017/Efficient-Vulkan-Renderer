#define MESHLETTRICOUNT 124

struct Vertex
{
    float vx, vy, vz;
    uint8_t nx, ny, nz, nw;
    float16_t tu, tv;
};

struct Meshlet
{
    vec4 cone;
    uint dataOffset;
	uint8_t triangleCount; 
	uint8_t vertexCount;
};

struct MeshDraw
{
    mat4 projection;
    // mat4 model;
    vec3 position;
    float scale;
    vec4 rotation;
};

vec3 rotate(vec3 pos, vec4 q)
{
    return pos + 2.0 * cross(q.xyz, cross(q.xyz, pos) + q.w * pos);
}