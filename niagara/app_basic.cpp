#include "app.h"

void renderApplication::createMeshes()
{
    meshes.resize(1);
    meshes[0].rtxSupported = rtxSupported;
    //meshes[0].loadMesh("..\\extern\\common-3d-test-models\\data\\xyzrgb_dragon.obj");
    meshes[0].loadMesh("..\\kitten.obj", rtxSupported);
    meshes[0].loadMesh("..\\extern\\common-3d-test-models\\data\\suzanne.obj", rtxSupported);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    meshes[0].generateRenderData(device, commandBuffers[0], graphicsQueue, memProperties);

    drawCount = 100000;
    float sceneRadius = 300.f;
    drawDistance = 300.f;

    draws.resize(drawCount);

    srand(32);

    for (uint32_t i = 0; i < drawCount; ++i)
    {
        int meshIndex = rand() % meshes[0].m_instances.size();
        const MeshInstance& mesh = meshes[0].m_instances[meshIndex];
        //draws[i].model = glm::mat4(1.f, 0.f, 0.f, 0.f,
        //    0.f, 1.f, 0.f, 0.f,
        //    0.f, 0.f, 1.f, 0.f,
        //    (float(rand()) / RAND_MAX) * 40.f - 20.f, (float(rand()) / RAND_MAX) * 40.f - 20.f, (float(rand()) / RAND_MAX) * 40.f - 20.f, 1.f);
        draws[i].position[0] = (float(rand()) / RAND_MAX) * sceneRadius * 2 - sceneRadius;
        draws[i].position[1] = (float(rand()) / RAND_MAX) * sceneRadius * 2 - sceneRadius;
        draws[i].position[2] = (float(rand()) / RAND_MAX) * sceneRadius * 2 - sceneRadius;
        draws[i].scale = meshIndex == 1 ? (float(rand()) / RAND_MAX) *  0.3f + 0.4f : (float(rand()) / RAND_MAX) + 1.f;

        glm::vec3 axis((float(rand()) / RAND_MAX) * 2.f - 1.f, (float(rand()) / RAND_MAX) * 2.f - 1.f, (float(rand()) / RAND_MAX) * 2.f - 1.f);
        float angle = glm::radians((float(rand()) / RAND_MAX) * 90.f);
        draws[i].rotation = glm::rotate(glm::quat(1.f, 0.f, 0.f, 0.f), angle, axis);

        draws[i].meshIndex = meshIndex;
        draws[i].vertexOffset = mesh.vertexOffset;

        //memset(draws[i].commandData, 0, sizeof(draws[i].commandData));
        //draws[i].commandIndirect.indexCount = uint32_t(mesh.indexCount);
        //draws[i].commandIndirect.firstIndex = mesh.indexOffset;
        //draws[i].commandIndirect.instanceCount = 1;
        //draws[i].commandIndirect.vertexOffset = mesh.vertexOffset;
        //draws[i].commandIndirectMS.taskCount = (mesh.meshletCount + 31) / 32;
        triangleCount += mesh.lods[0].indexCount / 3;
    }
    Buffer scratch = {};
    createBuffer(scratch, device, memProperties, sizeof(draws[0]) * draws.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    createBuffer(db, device, memProperties, sizeof(draws[0]) * draws.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    createBuffer(dcb, device, memProperties, sizeof(MeshDrawCommand) * draws.size(), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    createBuffer(dccb, device, memProperties, 4, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    uploadBuffer(device, commandBuffers[0], graphicsQueue, db, scratch, draws.data(), draws.size() * sizeof(MeshDraw));

    destroyBuffer(scratch, device);
}

void renderApplication::createInstance() {
    if (volkInitialize() != VK_SUCCESS)
    {
        throw std::runtime_error("failed to initialize volk");
    }

    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    volkLoadInstance(instance);
}

void renderApplication::createRenderPass() {
    VkAttachmentDescription attachments[2] = {};

    attachments[0].format = swapChainImageFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[1].format = VK_FORMAT_D32_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void renderApplication::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void renderApplication::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void renderApplication::createQueryPool()
{
    VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = QUERYCOUNT;

    if (vkCreateQueryPool(device, &createInfo, 0, &queryPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create query pool");
    }
}