#ifndef NIAGARA_COMMON_HELPER
#define NIAGARA_COMMON_HELPER
#include "niagara_prereq.h"

struct Buffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	void* data;
	size_t size;
};

uint32_t findMemoryType(const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void createBuffer(Buffer& result, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProperties, size_t size, VkBufferUsageFlags usage);

void destroyBuffer(const Buffer& buffer, VkDevice device);

VkImageMemoryBarrier imageBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout);

#endif