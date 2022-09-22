#ifndef MESH
#define MESH

#include "niagara_prereq.h"
#include "tiny_obj_loader.h"
#include "MeshOptimizer/meshoptimizer.h"
#include "common_helper.h"

// a simple & generic vertex layout
struct Vertex
{
	uint16_t px, py, pz, pw;
	uint8_t nx, ny, nz, nw;
	//glm::vec3 color;
	uint16_t tu, tv;

	static VkVertexInputBindingDescription getBindingDescription() 
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, px);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R8G8B8A8_UINT;
		attributeDescriptions[1].offset = offsetof(Vertex, nx);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R16G16_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, tu);

		return attributeDescriptions;
	}
};

struct alignas(16) Meshlet
{
	float cone[4];
	uint32_t vertices[64];
	uint8_t indices[MESHLETTRICOUNT * 3]; // up to MESHLETCOUNT triangles
	uint8_t triangleCount; 
	uint8_t vertexCount;
};

class Mesh
{
public:
	void loadMesh(std::string objpath);
	void generateRenderData(VkDevice device, VkCommandBuffer commandBuffer, VkQueue queue, const VkPhysicalDeviceMemoryProperties& memoryProperties);
	void destroyRenderData(VkDevice device);

	Buffer vb;
	Buffer ib;
	Buffer mb;

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	std::vector<Meshlet> m_meshlets;

	bool rtxSupported;
private:
	void buildMeshlets();
	void buildMeshletCones();
};

#endif