#define MESHLETTRICOUNT 124

struct Vertex
{
    float vx, vy, vz;
    uint8_t nx, ny, nz, nw;
    float16_t tu, tv;
};

struct Meshlet
{
    vec3 center;
    float radius;
    // vec3 cone_apex;
    // float padding;
    int8_t cone_axis[3];
    int8_t cone_cutoff;

    uint dataOffset;
	uint8_t triangleCount; 
	uint8_t vertexCount;
};

struct Globals
{
    mat4 projection;
};

struct MeshDraw
{
    vec3 position;
    float scale;
    vec4 rotation;

    uint vertexOffset;
    uint indexOffset;
	uint indexCount;
    uint meshletOffset;
    uint meshletCount;
};

struct MeshDrawCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
    uint taskCount;
    uint firstTask;
};

vec3 rotate(vec3 pos, vec4 q)
{
    return pos + 2.0 * cross(q.xyz, cross(q.xyz, pos) + q.w * pos);
}