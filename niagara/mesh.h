#ifndef MESH
#define MESH

#include "niagara_prereq.h"
#include "tiny_obj_loader.h"
#include "MeshOptimizer/meshoptimizer.h"
#include "common_helper.h"

// a simple & generic vertex layout
struct Vertex
{
	float px, py, pz;
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
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
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
	glm::vec3 center;
	float radius;
	//glm::vec3 cone_apex;
	//float padding;
	int8_t cone_axis[3];
	int8_t cone_cutoff;

	uint32_t dataOffset;
	uint8_t triangleCount; 
	uint8_t vertexCount;
};

struct alignas(16) Globals
{
	glm::mat4 projection;
};

struct alignas(16) MeshDraw
{
	//glm::mat4 model;
	glm::vec3 position;
	float scale;
	glm::quat rotation;

	uint32_t vertexOffset;
	uint32_t indexOffset;
	uint32_t indexCount;
	uint32_t meshletOffset;
	uint32_t meshletCount;
};

struct MeshDrawCommand
{
	VkDrawIndexedIndirectCommand indirect; // 5 * 4
	VkDrawMeshTasksIndirectCommandNV indirectMS; // 2 * 4
};

struct MeshInstance
{
	uint32_t meshletOffset;
	uint32_t meshletCount;

	uint32_t vertexOffset;
	uint32_t vertexCount;

	uint32_t indexOffset;
	uint32_t indexCount;
};

glm::mat4 MakeInfReversedZProjRH(float fovY_radians, float aspectWbyH, float zNear);

class Mesh
{
public:
	void loadMesh(std::string objpath, bool buildMeshlets);
	void generateRenderData(VkDevice device, VkCommandBuffer commandBuffer, VkQueue queue, const VkPhysicalDeviceMemoryProperties& memoryProperties);
	void destroyRenderData(VkDevice device);

	Buffer vb;
	Buffer ib;
	Buffer mb;
	Buffer mdb;

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	std::vector<Meshlet> m_meshlets;
	std::vector<uint32_t> m_meshlet_data;

	std::vector<MeshInstance> m_instances;

	// To Do: use meshlet_vertices & meshlet_indices buffers

	bool rtxSupported;
};

#endif