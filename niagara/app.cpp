#include "app.h"

bool meshShadingSwitch = true;
bool querySwitch = false;
bool cullSwitch = true;
bool lodSwitch = true;
bool debugPyramidSwitch = false;
uint32_t debugPyramidLevelInput = 0;

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
            meshShadingSwitch = !meshShadingSwitch;
        }
        if (key == GLFW_KEY_Q)
        {
            querySwitch = !querySwitch;
        }
        if (key == GLFW_KEY_C)
        {
            cullSwitch = !cullSwitch;
        }
        if (key == GLFW_KEY_L)
        {
            lodSwitch = !lodSwitch;
        }
        if (key == GLFW_KEY_P)
        {
            debugPyramidSwitch = !debugPyramidSwitch;
        }
        if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
        {
            debugPyramidLevelInput = key - GLFW_KEY_0;
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
    createRenderPass();
    createRenderPassLate();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
    createMeshes();
    createQueryPool();
}

void renderApplication::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        rtxEnabled = meshShadingSwitch;
        queryEnabled = querySwitch;
        cullEnabled = cullSwitch;
        lodEnabled = lodSwitch;
        debugPyramid = debugPyramidSwitch;
        debugPyramidLevel = debugPyramidLevelInput;
        double frameCPUBegin = glfwGetTime() * 1000;
        glfwPollEvents();
        drawFrame();
        double frameCPUEnd = glfwGetTime() * 1000;
        frameCPUAvg = frameCPUAvg * 0.95 + (frameCPUEnd - frameCPUBegin) * 0.05;
        double trianglesPerSec = frameGPUAvg > 0.f ? double(triangleCount) / double(frameGPUAvg * 1e-3) : 0.f;
        double meshPerSec = frameGPUAvg > 0.f ? double(drawCount) / double(frameGPUAvg * 1e-3) : 0.f;
        char title[256];
        sprintf(title, "cpu: %.1f ms; gpu: %.3f ms (cull: %.2f ms); triangles %.1fM; mesh shading %s; %.1fB tri/sec; show query %s; culling %s; lod %s",
            frameCPUAvg, frameGPUAvg, cullGPUTime, double(triangleCount) * 1e-6, rtxEnabled ? "ON" : "OFF", 
            trianglesPerSec * 1e-9, queryEnabled ? "ON" : "OFF", cullEnabled ? "ON" : "OFF", lodEnabled ? "ON" : "OFF");
        glfwSetWindowTitle(window, title);
    }

    vkDeviceWaitIdle(device);
}

void renderApplication::cleanup() {
    vkDestroyQueryPool(device, queryPool, nullptr);
    vkDestroyQueryPool(device, pipeStatsQueryPool, nullptr);
    for (size_t i = 0; i < meshes.size(); ++i)
    {
        meshes[i].destroyRenderData(device);
    }

    destroyShader(drawcullCS);
    destroyShader(depthreduceCS);

    vkDestroySampler(device, depthSampler, nullptr);

    destroyBuffer(db, device);
    destroyBuffer(dcb, device);
    destroyBuffer(dccb, device);

    if (depthPyramid.image)
    {
        for (uint32_t i = 0; i < depthPyramidLevels; ++i)
        {
            vkDestroyImageView(device, depthPyramidMips[i], 0);
        }
        destroyImage(depthPyramid, device);
    }

    destroyImage(colorTarget, device);
    destroyImage(depthTarget, device);
    vkDestroyFramebuffer(device, targetFB, 0);

    cleanupSwapChain();

    if (rtxSupported)
    {
        vkDestroyPipeline(device, rtxGraphicsPipeline, nullptr);
        destroyProgram(rtxGraphicsProgram);
    }

    vkDestroyPipeline(device, depthreducePipeline, nullptr);
    destroyProgram(depthreduceProgram);

    vkDestroyPipeline(device, drawcmdPipeline, nullptr);
    destroyProgram(drawcmdProgram);

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    destroyProgram(graphicsProgram);

    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyRenderPass(device, renderPassLate, nullptr);

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