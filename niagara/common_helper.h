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

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct DescriptorInfo
{
    union
    {
        VkDescriptorImageInfo image;
        VkDescriptorBufferInfo buffer;
    };

    DescriptorInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
    {
        image.sampler = sampler;
        image.imageView = imageView;
        image.imageLayout = imageLayout;
    }

    DescriptorInfo(VkBuffer _buffer, VkDeviceSize offset, VkDeviceSize range)
    {
        buffer.buffer = _buffer;
        buffer.offset = offset;
        buffer.range = range;
    }

    DescriptorInfo(VkBuffer _buffer)
    {
        buffer.buffer = _buffer;
        buffer.offset = 0;
        buffer.range = VK_WHOLE_SIZE;
    }
};

struct Shader
{
    VkShaderModule module;
    VkShaderStageFlagBits stage;
    uint32_t storageBufferMask;

    bool usesPushConstant;
};

struct Program
{
    VkPipelineLayout layout;
    VkDescriptorUpdateTemplate updateTemplate;
    VkDescriptorSetLayout setLayout;

    VkShaderStageFlags pushConstantStages;
};

using Shaders = std::initializer_list<const Shader*>;

struct Id
{
    enum Kind { Unknown, Variable };

    Kind kind = Unknown;
    uint32_t type;
    uint32_t storageClass;
    uint32_t binding;
    uint32_t set;
};

uint32_t findMemoryType(const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void createBuffer(Buffer& result, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProperties, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags);

void uploadBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkQueue queue, const Buffer& buffer, const Buffer& scratch, const void* data, size_t size);

void destroyBuffer(const Buffer& buffer, VkDevice device);

VkImageMemoryBarrier imageBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout);

VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);

#endif