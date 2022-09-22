#include "app.h"

bool rtxSwitch = false;

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_R)
        {
            rtxSwitch = !rtxSwitch;
        }
    }
}

void renderApplication::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void renderApplication::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void renderApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<renderApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void renderApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
    createMeshes();
    createQueryPool();
}

void renderApplication::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        rtxEnabled = rtxSwitch;
        double frameCPUBegin = glfwGetTime() * 1000;
        glfwPollEvents();
        drawFrame();
        double frameCPUEnd = glfwGetTime() * 1000;
        frameCPUAvg = frameCPUAvg * 0.95 + (frameCPUEnd - frameCPUBegin) * 0.05;
        double trianglesPerSec = double(drawCount) * double(meshes[0].m_indices.size() / 3) / double(frameGPUAvg * 1e-3);
        char title[256];
        sprintf(title, "cpu: %.1f ms; gpu: %.3f ms; triangles %d; meshlets %d; mesh shading %s; %.1fB tri/sec",
            frameCPUAvg, frameGPUAvg, meshes[0].m_indices.size() / 3, meshes[0].m_meshlets.size(), rtxEnabled ? "ON" : "OFF", trianglesPerSec * 1e-9);
        glfwSetWindowTitle(window, title);
    }

    vkDeviceWaitIdle(device);
}

void renderApplication::cleanup() {
    vkDestroyQueryPool(device, queryPool, nullptr);
    for (size_t i = 0; i < meshes.size(); ++i)
    {
        meshes[i].destroyRenderData(device);
    }

    cleanupSwapChain();

    if (rtxSupported)
    {
        vkDestroyDescriptorUpdateTemplate(device, rtxUpdateTemplate, nullptr);
        vkDestroyDescriptorSetLayout(device, rtxSetLayout, nullptr);
        vkDestroyPipeline(device, rtxGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, rtxPipelineLayout, nullptr);
    }

    vkDestroyDescriptorUpdateTemplate(device, updateTemplate, nullptr);
    vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}