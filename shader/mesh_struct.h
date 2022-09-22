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