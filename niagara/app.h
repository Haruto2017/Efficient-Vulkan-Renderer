#ifndef NIAGARA_APP
#define NIAGARA_APP

#include "niagara_prereq.h"
#include "common_helper.h"
#include "mesh.h"

const uint32_t WIDTH = 1600;
const uint32_t HEIGHT = 1200;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
    VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
    VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

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

class renderApplication {
public:
    void run();

private:
    GLFWwindow* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkDescriptorSetLayout setLayout;
    VkDescriptorUpdateTemplate updateTemplate;

    VkPipelineLayout rtxPipelineLayout;
    VkPipeline rtxGraphicsPipeline;
    VkDescriptorSetLayout rtxSetLayout;
    VkDescriptorUpdateTemplate rtxUpdateTemplate;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    std::vector<Mesh> meshes;

    VkQueryPool queryPool;
    uint64_t queryResults[2];
    float timestampPeriod;
    double frameGPUBegin;
    double frameGPUEnd;

    double frameCPUAvg;
    double frameGPUAvg;

    bool rtxSupported = false;
    bool rtxEnabled = false;

    void initWindow();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void initVulkan();

    void mainLoop();

    void cleanupSwapChain();

    void cleanup();

    void recreateSwapChain();

    void createMeshes();

    void createInstance();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();

    void createSurface();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapChain();

    void createImageViews();

    void createRenderPass();

    void createGenericGraphicsPipelineLayout(VkPipelineLayout& outPipelineLayout, VkDescriptorSetLayout inSetLayout, bool rtxEnabled);

    void createGenericGraphicsPipeline(VkShaderModule vs, VkShaderModule fs, VkPipelineLayout inPipelineLayout, VkPipeline& outPipeline, bool rtxEnabled);

    void createGraphicsPipeline();

    void createSetLayout(VkDescriptorSetLayout& outLayout, bool rtxEnabled);

    void createUpdateTemplate(VkDescriptorUpdateTemplate& outTemplate, VkPipelineBindPoint bindPoint, VkPipelineLayout inLayout, VkDescriptorSetLayout inSetLayout, bool rtxEnabled);

    void createFramebuffers();

    void createCommandPool();

    void createCommandBuffers();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void createSyncObjects();

    void createQueryPool();

    void drawFrame();

    VkShaderModule createShaderModule(const std::vector<char>& code);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    bool isDeviceSuitable(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    std::vector<const char*> getRequiredExtensions();

    bool checkValidationLayerSupport();

    static std::vector<char> readFile(const std::string& filename);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};

#endif