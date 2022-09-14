#ifndef MESH
#define MESH

#include "niagara_prereq.h"
#include "tiny_obj_loader.h"
#include "meshoptimizer.h"
#include "common_helper.h"

// a simple & generic vertex layout
struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	//glm::vec3 color;
	glm::vec2 texcoord;

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
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texcoord);

		return attributeDescriptions;
	}
};

class Mesh
{
public:
	void loadMesh(std::string objpath);
	void generateRenderData(VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProperties);
	void destroyRenderData(VkDevice device);

	Buffer vb;
	Buffer ib;

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
};

#endif