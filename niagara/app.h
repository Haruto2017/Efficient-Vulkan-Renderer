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
    VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
    VK_KHR_MAINTENANCE_4_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

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

    Image colorTarget;
    Image depthTarget;
    VkFramebuffer targetFB;

    VkRenderPass renderPass;

    VkPipelineCache pipelineCache = 0;

    VkPipeline graphicsPipeline;
    Program graphicsProgram;

    VkPipeline rtxGraphicsPipeline;
    Program rtxGraphicsProgram;

    VkPipeline drawcmdPipeline;
    Program drawcmdProgram;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    std::vector<Mesh> meshes;
    std::vector<MeshDraw> draws;

    VkQueryPool queryPool;
    uint64_t queryResults[4];
    bool queryEnabled = false;
    float timestampPeriod;
    double frameGPUBegin;
    double frameGPUEnd;

    double frameCPUAvg;
    double frameGPUAvg;

    double cullGPUTime;

    uint32_t drawCount = 100;
    uint32_t triangleCount = 0;
    Buffer db;
    Buffer dcb;
    Buffer dccb;

    bool rtxSupported = false;
    bool rtxEnabled = false;

    bool cullEnabled = false;
    bool lodEnabled = false;

    float drawDistance;

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

    void createRenderPass();

    void createGenericGraphicsPipelineLayout(Shaders shaders, VkShaderStageFlags pushConstantStages, VkPipelineLayout& outPipelineLayout, VkDescriptorSetLayout inSetLayout, size_t pushConstantSize);

    void createGenericGraphicsPipeline(Shaders shaders, VkPipelineCache pipelineCache, VkPipelineLayout inPipelineLayout, VkPipeline& outPipeline);

    void createComputePipeline(VkPipelineCache pipelineCache, const Shader& shader, VkPipelineLayout inPipelineLayout, VkPipeline& outPipeline);
    
    void createGenericProgram(VkPipelineBindPoint bindPoint, Shaders shaders, size_t pushConstantSize, Program& outProgram);

    void destroyProgram(const Program& program);
    
    void createGraphicsPipeline();

    void createSetLayout(Shaders shaders, VkDescriptorSetLayout& outLayout);

    void createUpdateTemplate(Shaders shaders, VkDescriptorUpdateTemplate& outTemplate, VkPipelineBindPoint bindPoint, VkPipelineLayout inLayout);

    void createCommandPool();

    void createCommandBuffers();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void createSyncObjects();

    void createQueryPool();

    void drawFrame();

    bool createShader(Shader& shader, const std::vector<char>& code);

    void destroyShader(Shader& shader);

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