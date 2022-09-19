#define MESHLETTRICOUNT 84

struct Vertex
{
    float16_t vx, vy, vz, vw;
    uint8_t nx, ny, nz, nw;
    float16_t tu, tv;
};

struct Meshlet
{
	uint vertices[64];
	uint8_t indices[MESHLETTRICOUNT*3]; // up to 126 triangles
    //uint indicesPacked[MESHLETTRICOUNT*3 / 4];
	uint8_t triangleCount; 
	uint8_t vertexCount;
};