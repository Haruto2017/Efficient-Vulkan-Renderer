#ifndef MESH
#define MESH

#define TINYOBJLOADER_IMPLEMENTATION

#include "niagara_prereq.h"
#include "tiny_obj_loader.h"

// a simple & generic vertex layout
struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	//glm::vec3 color;
	glm::vec2 texcoord;
};

class Mesh
{
public:
	void loadMesh(std::string objpath);

private:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

#endif