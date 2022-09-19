#include "app.h"

static VkShaderStageFlagBits getShaderStage(SpvExecutionModel executionModel)
{
    switch (executionModel)
    {
    case SpvExecutionModelVertex:
    {
        return VK_SHADER_STAGE_VERTEX_BIT;
    } break;
    case SpvExecutionModelFragment:
    {
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    } break;
    case SpvExecutionModelTaskNV:
    {
        return VK_SHADER_STAGE_TASK_BIT_NV;
    } break;
    case SpvExecutionModelMeshNV:
    {
        return VK_SHADER_STAGE_MESH_BIT_NV;
    } break;
    default:
        throw std::runtime_error("Unsupported execution model");
        return VkShaderStageFlagBits(0);
    }
}

static void parseShader(Shader& shader, const uint32_t* code, uint32_t codeSize)
{
    if (code[0] != SpvMagicNumber)
    {
        throw std::runtime_error("code[0] != SpvMagicNumber");
    }

    uint32_t idBound = code[3];

    const uint32_t* insn = code + 5;

    while (insn != code + codeSize)
    {
        uint16_t opcode = uint16_t(insn[0] & 0xffff);
        uint16_t wordCount = uint16_t(insn[0] >> 16);

        switch (opcode)
        {
        case SpvOpEntryPoint:
        {
            shader.stage = getShaderStage(SpvExecutionModel(insn[1]));
        } break;
        }

        if (!(insn + wordCount <= code + codeSize))
        {
            throw std::runtime_error("!(insn + wordCount <= code + codeSize)");
        }
        insn += wordCount;
    }
}

bool renderApplication::createShader(Shader& shader, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    parseShader(shader, reinterpret_cast<const uint32_t*>(code.data()), code.size() / 4);

    shader.module = shaderModule;
    // result.stage

    return true;
}

void renderApplication::destroyShader(Shader& shader)
{
    vkDestroyShaderModule(device, shader.module, 0);
}

std::vector<char> renderApplication::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

void renderApplication::createGenericGraphicsPipelineLayout(VkPipelineLayout& outPipelineLayout, VkDescriptorSetLayout inSetLayout, bool rtxEnabled)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &inSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &outPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void renderApplication::createGenericGraphicsPipeline(const Shader& vs, const Shader& fs, VkPipelineLayout inPipelineLayout, VkPipeline& outPipeline, bool rtxEnabled)
{
    if (!(vs.stage == VK_SHADER_STAGE_VERTEX_BIT || vs.stage == VK_SHADER_STAGE_MESH_BIT_NV))
    {
        throw std::runtime_error("wrong shader stage");
    }
    if (!(fs.stage == VK_SHADER_STAGE_FRAGMENT_BIT))
    {
        throw std::runtime_error("wrong shader stage");
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    vertShaderStageInfo.stage = vs.stage;

    vertShaderStageInfo.module = vs.module;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = fs.stage;
    fragShaderStageInfo.module = fs.module;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = inPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}

void renderApplication::createGraphicsPipeline() {
    bool rc = false;
    std::vector<char> meshShaderCode;
    Shader meshShader = {};
    if (rtxSupported)
    {
        meshShaderCode = readFile("..\\compiledShader\\meshlet.mesh.spv");
        if (!createShader(meshShader, meshShaderCode))
        {
            throw std::runtime_error("failed to create mesh shader");
        }
    }

    auto vertShaderCode = readFile("..\\compiledShader\\simple.vert.spv");

    auto fragShaderCode = readFile("..\\compiledShader\\simple.frag.spv");

    Shader vertShader = {};
    if (!createShader(vertShader, vertShaderCode))
    {
        throw std::runtime_error("failed to create vert shader");
    }
    Shader fragShader = {};
    if (!createShader(fragShader, fragShaderCode))
    {
        throw std::runtime_error("failed to create frag shader");
    }

    if (rtxSupported)
    {
        createSetLayout(rtxSetLayout, true);
        createGenericGraphicsPipelineLayout(rtxPipelineLayout, rtxSetLayout, true);
        createUpdateTemplate(rtxUpdateTemplate, VK_PIPELINE_BIND_POINT_GRAPHICS, rtxPipelineLayout, rtxSetLayout,true);
        createGenericGraphicsPipeline(meshShader, fragShader, rtxPipelineLayout, rtxGraphicsPipeline, true);
    }
    createSetLayout(setLayout, false);
    createGenericGraphicsPipelineLayout(pipelineLayout, setLayout, false);
    createUpdateTemplate(updateTemplate, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, setLayout, false);
    createGenericGraphicsPipeline(vertShader, fragShader, pipelineLayout, graphicsPipeline, false);

    destroyShader(fragShader);
    destroyShader(vertShader);
    if (rtxSupported)
    {
        destroyShader(meshShader);
    }
}

void renderApplication::createSetLayout(VkDescriptorSetLayout& outLayout, bool rtxEnabled)
{
    std::vector<VkDescriptorSetLayoutBinding> setBindings;

    if (rtxEnabled)
    {
        setBindings.resize(2);
        setBindings[0].binding = 0;
        setBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        setBindings[0].descriptorCount = 1;
        setBindings[0].stageFlags = VK_SHADER_STAGE_MESH_BIT_NV;

        setBindings[1].binding = 1;
        setBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        setBindings[1].descriptorCount = 1;
        setBindings[1].stageFlags = VK_SHADER_STAGE_MESH_BIT_NV;
    }
    else
    {
        setBindings.resize(1);
        setBindings[0].binding = 0;
        setBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        setBindings[0].descriptorCount = 1;
        setBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    }

    VkDescriptorSetLayoutCreateInfo setCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    setCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;

    setCreateInfo.bindingCount = setBindings.size();
    setCreateInfo.pBindings = setBindings.data();


    if (vkCreateDescriptorSetLayout(device, &setCreateInfo, 0, &outLayout))
    {
        throw std::runtime_error("failed to create descriptor layout!");
    }
}

void renderApplication::createUpdateTemplate(VkDescriptorUpdateTemplate& outTemplate, VkPipelineBindPoint bindPoint, VkPipelineLayout inLayout, VkDescriptorSetLayout inSetLayout, bool rtxEnabled)
{
    std::vector<VkDescriptorUpdateTemplateEntry> entries;

    if (rtxEnabled)
    {
        entries.resize(2);
        entries[0].dstBinding = 0;
        entries[0].dstArrayElement = 0;
        entries[0].descriptorCount = 1;
        entries[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        entries[0].offset = sizeof(DescriptorInfo) * 0;
        entries[0].stride = sizeof(DescriptorInfo);

        entries[1].dstBinding = 1;
        entries[1].dstArrayElement = 0;
        entries[1].descriptorCount = 1;
        entries[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        entries[1].offset = sizeof(DescriptorInfo) * 1;
        entries[1].stride = sizeof(DescriptorInfo);
    }
    else
    {
        entries.resize(1);
        entries[0].dstBinding = 0;
        entries[0].dstArrayElement = 0;
        entries[0].descriptorCount = 1;
        entries[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        entries[0].offset = sizeof(DescriptorInfo) * 0;
        entries[0].stride = sizeof(DescriptorInfo);
    }

    VkDescriptorUpdateTemplateCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO };

    createInfo.descriptorUpdateEntryCount = uint32_t(entries.size());
    createInfo.pDescriptorUpdateEntries = entries.data();

    createInfo.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR;
    createInfo.pipelineBindPoint = bindPoint;
    createInfo.pipelineLayout = inLayout;
    createInfo.descriptorSetLayout = inSetLayout;
    if (vkCreateDescriptorUpdateTemplate(device, &createInfo, 0, &outTemplate) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create update template");
    }
}