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

    std::vector<Id> ids(idBound);

    const uint32_t* insn = code + 5;

    while (insn != code + codeSize)
    {
        uint16_t opcode = uint16_t(insn[0] & 0xffff);
        uint16_t wordCount = uint16_t(insn[0] >> 16);

        switch (opcode)
        {
        case SpvOpEntryPoint:
        {
            if (wordCount < 2)
            {
                throw std::runtime_error("bad word count");
            }
            shader.stage = getShaderStage(SpvExecutionModel(insn[1]));
        } break;
        case SpvOpDecorate:
        {
            if (wordCount < 3)
            {
                throw std::runtime_error("bad word count");
            }
            uint32_t id = insn[1];

            switch (insn[2])
            {
            case SpvDecorationDescriptorSet:
                ids[id].set = insn[3];
                break;
            case SpvDecorationBinding:
                ids[id].binding = insn[3];
                int a = 0;
                break;
            }

        } break;
        case SpvOpVariable:
        {
            if (wordCount < 2)
            {
                throw std::runtime_error("bad word count");
            }
            uint32_t id = insn[2];

            ids[id].kind = Id::Variable;
            ids[id].type = insn[1];
            ids[id].storageClass = insn[3];
        } break;
        }

        if (!(insn + wordCount <= code + codeSize))
        {
            throw std::runtime_error("!(insn + wordCount <= code + codeSize)");
        }
        insn += wordCount;
    }

    for (auto& id : ids)
    {
        if (id.kind == Id::Variable && id.storageClass == SpvStorageClassStorageBuffer)
        {
            assert(id.set == 0);
            assert(id.binding < 32);
            assert((shader.storageBufferMask & (1 << id.binding)) == 0);

            shader.storageBufferMask |= 1 << id.binding;
        }
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

void renderApplication::createGenericGraphicsPipelineLayout(Shaders shaders, VkPipelineLayout& outPipelineLayout, VkDescriptorSetLayout inSetLayout, size_t pushConstantSize)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &inSetLayout;

    VkPushConstantRange pushConstantRange = {};

    if (pushConstantSize)
    {
        pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
        pushConstantRange.offset = 0;
        pushConstantRange.size = pushConstantSize;

        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    }

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &outPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void renderApplication::createGenericGraphicsPipeline(Shaders shaders, VkPipelineLayout inPipelineLayout, VkPipeline& outPipeline)
{
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    for (const Shader* shader : shaders)
    {
        VkPipelineShaderStageCreateInfo stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stage.stage = shader->stage;;
        stage.module = shader->module;
        stage.pName = "main";
        stages.push_back(stage);
    }

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
#if VISUALIZE_BACK_CULLING
    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
#else
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
#endif
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    pipelineInfo.stageCount = uint32_t(stages.size());
    pipelineInfo.pStages = stages.data();
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

void renderApplication::createSetLayout(Shaders shaders, VkDescriptorSetLayout& outLayout)
{
    std::vector<VkDescriptorSetLayoutBinding> setBindings;

    uint32_t storageBufferMask = 0;
    for (const Shader* shader : shaders)
    {
        storageBufferMask |= shader->storageBufferMask;
    }

    for (uint32_t i = 0; i < 32; ++i)
    {
        if (storageBufferMask & (1 << i))
        {
            VkDescriptorSetLayoutBinding binding = {};
            binding.binding = i;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            binding.descriptorCount = 1;

            binding.stageFlags = 0;
            for (const Shader* shader : shaders)
            {
                if (shader->storageBufferMask & (1 << i))
                {
                    binding.stageFlags |= shader->stage;
                }
            }

            setBindings.push_back(binding);
        }
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

void renderApplication::createUpdateTemplate(Shaders shaders, VkDescriptorUpdateTemplate& outTemplate, VkPipelineBindPoint bindPoint, VkPipelineLayout inLayout)
{
    std::vector<VkDescriptorUpdateTemplateEntry> entries;

    uint32_t storageBufferMask = 0;
    for (const Shader* shader : shaders)
    {
        storageBufferMask |= shader->storageBufferMask;
    }

    for (uint32_t i = 0; i < 32; ++i)
    {
        if (storageBufferMask & (1 << i))
        {
            VkDescriptorUpdateTemplateEntry entry = {};
            entry.dstBinding = i;
            entry.dstArrayElement = 0;
            entry.descriptorCount = 1;
            entry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            entry.offset = sizeof(DescriptorInfo) * i;
            entry.stride = sizeof(DescriptorInfo);

            entries.push_back(entry);
        }
    }

    VkDescriptorUpdateTemplateCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO };

    createInfo.descriptorUpdateEntryCount = uint32_t(entries.size());
    createInfo.pDescriptorUpdateEntries = entries.data();

    createInfo.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR;
    createInfo.pipelineBindPoint = bindPoint;
    createInfo.pipelineLayout = inLayout;
    //createInfo.descriptorSetLayout = inSetLayout;
    if (vkCreateDescriptorUpdateTemplate(device, &createInfo, 0, &outTemplate) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create update template");
    }
}

void renderApplication::createGraphicsPipeline() {
    bool rc = false;
    Shader meshShader = {};
    Shader taskShader = {};
    if (rtxSupported)
    {
        std::vector<char> meshShaderCode = readFile("..\\compiledShader\\meshlet.mesh.spv");
        if (!createShader(meshShader, meshShaderCode))
        {
            throw std::runtime_error("failed to create mesh shader");
        }
        std::vector<char> taskShaderCode = readFile("..\\compiledShader\\meshlet.task.spv");
        if (!createShader(taskShader, taskShaderCode))
        {
            throw std::runtime_error("failed to create task shader");
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
        createSetLayout({ &taskShader, &meshShader, &fragShader }, rtxSetLayout);
        createGenericGraphicsPipelineLayout({ &taskShader, &meshShader, &fragShader }, rtxPipelineLayout, rtxSetLayout, sizeof(MeshDraw));
        createUpdateTemplate({ &taskShader, &meshShader, &fragShader }, rtxUpdateTemplate, VK_PIPELINE_BIND_POINT_GRAPHICS, rtxPipelineLayout);
        createGenericGraphicsPipeline({ &taskShader, &meshShader, &fragShader }, rtxPipelineLayout, rtxGraphicsPipeline);
    }
    createSetLayout({ &vertShader, &fragShader }, setLayout);
    createGenericGraphicsPipelineLayout({ &vertShader, &fragShader }, pipelineLayout, setLayout, sizeof(MeshDraw));
    createUpdateTemplate({ &vertShader, &fragShader }, updateTemplate, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout);
    createGenericGraphicsPipeline({ &vertShader, &fragShader }, pipelineLayout, graphicsPipeline);

    destroyShader(fragShader);
    destroyShader(vertShader);
    if (rtxSupported)
    {
        destroyShader(meshShader);
        destroyShader(taskShader);
    }
}