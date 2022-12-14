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
    case SpvExecutionModelGLCompute:
    {
        return VK_SHADER_STAGE_COMPUTE_BIT;
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

static VkDescriptorType getDescriptorType(SpvOp op)
{
    switch (op)
    {
    case SpvOpTypeStruct:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case SpvOpTypeImage:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case SpvOpTypeSampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case SpvOpTypeSampledImage:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    default:
        assert(!"Unknown resource type");
        return VkDescriptorType(0);
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

    int localSizeIdX = -1;
    int localSizeIdY = -1;
    int localSizeIdZ = -1;

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
        case SpvOpExecutionModeId:
        {
            if (wordCount < 3)
            {
                throw std::runtime_error("bad word count");
            }
            uint32_t mode = insn[2];

            switch (mode)
            {
            case SpvExecutionModeLocalSizeId:
                assert(wordCount == 6);
                localSizeIdX = int(insn[3]);
                localSizeIdY = int(insn[4]);
                localSizeIdZ = int(insn[5]);
                break;
            }
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
        case SpvOpTypeStruct:
        case SpvOpTypeImage:
        case SpvOpTypeSampler:
        case SpvOpTypeSampledImage:
        {
            assert(wordCount >= 2);

            uint32_t id = insn[1];
            assert(id < idBound);

            assert(ids[id].opcode == 0);
            ids[id].opcode = opcode;
        } break;
        case SpvOpTypePointer:
        {
            assert(wordCount == 4);

            uint32_t id = insn[1];
            assert(id < idBound);

            assert(ids[id].opcode == 0);
            ids[id].opcode = opcode;
            ids[id].typeId = insn[3];
            ids[id].storageClass = insn[2];
        } break;
        case SpvOpConstant:
        {
            assert(wordCount >= 4); // we currently only correctly handle 32-bit integer constants

            uint32_t id = insn[2];
            assert(id < idBound);

            assert(ids[id].opcode == 0);
            ids[id].opcode = opcode;
            ids[id].typeId = insn[1];
            ids[id].constant = insn[3]; // note: this is the value, not the id of the constant
        } break;
        case SpvOpVariable:
        {
            assert(wordCount >= 4);

            uint32_t id = insn[2];
            assert(id < idBound);

            assert(ids[id].opcode == 0);
            ids[id].opcode = opcode;
            ids[id].typeId = insn[1];
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
        if (id.opcode == SpvOpVariable && (id.storageClass == SpvStorageClassUniform || id.storageClass == SpvStorageClassUniformConstant || id.storageClass == SpvStorageClassStorageBuffer))
        {
            assert(id.set == 0);
            assert(id.binding < 32);
            assert(ids[id.typeId].opcode == SpvOpTypePointer);

            uint32_t typeKind = ids[ids[id.typeId].typeId].opcode;
            VkDescriptorType resourceType = getDescriptorType(SpvOp(typeKind));

            assert((shader.resourceMask & (1 << id.binding)) == 0 || shader.resourceTypes[id.binding] == resourceType);

            shader.resourceTypes[id.binding] = resourceType;
            shader.resourceMask |= 1 << id.binding;
        }

        if (id.opcode == SpvOpVariable && id.storageClass == SpvStorageClassPushConstant)
        {
            shader.usesPushConstant = true;
        }
    }

    if (shader.stage == VK_SHADER_STAGE_COMPUTE_BIT)
    {
        if (localSizeIdX >= 0)
        {
            assert(ids[localSizeIdX].opcode == SpvOpConstant);
            shader.localSizeX = ids[localSizeIdX].constant;
        }

        if (localSizeIdY >= 0)
        {
            assert(ids[localSizeIdY].opcode == SpvOpConstant);
            shader.localSizeY = ids[localSizeIdY].constant;
        }

        if (localSizeIdZ >= 0)
        {
            assert(ids[localSizeIdZ].opcode == SpvOpConstant);
            shader.localSizeZ = ids[localSizeIdZ].constant;
        }

        assert(shader.localSizeX && shader.localSizeY && shader.localSizeZ);
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

void renderApplication::createGenericGraphicsPipelineLayout(Shaders shaders, VkShaderStageFlags pushConstantStages, VkPipelineLayout& outPipelineLayout, VkDescriptorSetLayout inSetLayout, size_t pushConstantSize)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &inSetLayout;

    VkPushConstantRange pushConstantRange = {};

    if (pushConstantSize)
    {
        pushConstantRange.stageFlags = pushConstantStages;
        pushConstantRange.offset = 0;
        pushConstantRange.size = pushConstantSize;

        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    }

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &outPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void renderApplication::createGenericGraphicsPipeline(Shaders shaders, VkPipelineCache pipelineCache, VkPipelineLayout inPipelineLayout, VkPipeline& outPipeline)
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
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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

    VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencilState.depthTestEnable = true;
    depthStencilState.depthWriteEnable = true;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER;

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
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.layout = inPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &outPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}

void renderApplication::createComputePipeline(VkPipelineCache pipelineCache, const Shader& shader, VkPipelineLayout inPipelineLayout, VkPipeline& outPipeline)
{
    assert(shader.stage == VK_SHADER_STAGE_COMPUTE_BIT);

    VkPipelineShaderStageCreateInfo stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    stage.stage = shader.stage;
    stage.module = shader.module;
    stage.pName = "main";

    VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    createInfo.layout = inPipelineLayout;
    createInfo.stage = stage;

    if (vkCreateComputePipelines(device, pipelineCache, 1, &createInfo, 0, &outPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }
}

void renderApplication::createGenericProgram(VkPipelineBindPoint bindPoint, Shaders shaders, size_t pushConstantSize, Program& outProgram)
{
    VkShaderStageFlags pushConstantStages = 0;
    for (const Shader* shader : shaders)
    {
        if (shader->usesPushConstant)
        {
            pushConstantStages |= shader->stage;
        }
    }
    outProgram.pushConstantStages = pushConstantStages;
    createSetLayout(shaders, outProgram.setLayout);
    createGenericGraphicsPipelineLayout(shaders, pushConstantStages ,outProgram.layout, outProgram.setLayout, pushConstantSize);
    createUpdateTemplate(shaders, outProgram.updateTemplate, bindPoint, outProgram.layout);
}

void renderApplication::destroyProgram(const Program& program)
{
    vkDestroyDescriptorUpdateTemplate(device, program.updateTemplate, nullptr);
    vkDestroyDescriptorSetLayout(device, program.setLayout, nullptr);
    vkDestroyPipelineLayout(device, program.layout, nullptr);
}

static uint32_t gatherResources(Shaders shaders, VkDescriptorType(&resourceTypes)[32])
{
    uint32_t resourceMask = 0;

    for (const Shader* shader : shaders)
    {
        for (uint32_t i = 0; i < 32; ++i)
        {
            if (shader->resourceMask & (1 << i))
            {
                if (resourceMask & (1 << i))
                {
                    assert(resourceTypes[i] == shader->resourceTypes[i]);
                }
                else
                {
                    resourceTypes[i] = shader->resourceTypes[i];
                    resourceMask |= 1 << i;
                }
            }
        }
    }

    return resourceMask;
}

void renderApplication::createSetLayout(Shaders shaders, VkDescriptorSetLayout& outLayout)
{
    std::vector<VkDescriptorSetLayoutBinding> setBindings;

    VkDescriptorType resourceTypes[32] = {};
    uint32_t resourceMask = gatherResources(shaders, resourceTypes);

    for (uint32_t i = 0; i < 32; ++i)
        if (resourceMask & (1 << i))
        {
            VkDescriptorSetLayoutBinding binding = {};
            binding.binding = i;
            binding.descriptorType = resourceTypes[i];
            binding.descriptorCount = 1;

            binding.stageFlags = 0;
            for (const Shader* shader : shaders)
                if (shader->resourceMask & (1 << i))
                    binding.stageFlags |= shader->stage;

            setBindings.push_back(binding);
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

    VkDescriptorType resourceTypes[32] = {};
    uint32_t resourceMask = gatherResources(shaders, resourceTypes);

    for (uint32_t i = 0; i < 32; ++i)
        if (resourceMask & (1 << i))
        {
            VkDescriptorUpdateTemplateEntry entry = {};
            entry.dstBinding = i;
            entry.dstArrayElement = 0;
            entry.descriptorCount = 1;
            entry.descriptorType = resourceTypes[i];
            entry.offset = sizeof(DescriptorInfo) * i;
            entry.stride = sizeof(DescriptorInfo);

            entries.push_back(entry);
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
    std::vector<char> compShaderCode = readFile("..\\compiledShader\\drawcmd.comp.spv");
    if (!createShader(drawcullCS, compShaderCode))
    {
        throw std::runtime_error("failed to create comp shader");
    }

    std::vector<char> depthreduceShaderCode = readFile("..\\compiledShader\\depthreduce.comp.spv");
    if (!createShader(depthreduceCS, depthreduceShaderCode))
    {
        throw std::runtime_error("failed to create comp shader");
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
        createGenericProgram(VK_PIPELINE_BIND_POINT_GRAPHICS, { &taskShader, &meshShader, &fragShader }, sizeof(Globals), rtxGraphicsProgram);
        createGenericGraphicsPipeline({ &taskShader, &meshShader, &fragShader }, pipelineCache, rtxGraphicsProgram.layout, rtxGraphicsPipeline);
    }

    createGenericProgram(VK_PIPELINE_BIND_POINT_COMPUTE, { &drawcullCS }, sizeof(DrawCullData), drawcmdProgram);
    createComputePipeline(pipelineCache, drawcullCS, drawcmdProgram.layout, drawcmdPipeline);

    createGenericProgram(VK_PIPELINE_BIND_POINT_COMPUTE, { &depthreduceCS }, sizeof(DepthReduceData), depthreduceProgram);
    createComputePipeline(pipelineCache, depthreduceCS, depthreduceProgram.layout, depthreducePipeline);

    depthSampler = createSampler(device);

    createGenericProgram(VK_PIPELINE_BIND_POINT_GRAPHICS, { &vertShader, &fragShader }, sizeof(Globals), graphicsProgram);
    createGenericGraphicsPipeline({ &vertShader, &fragShader }, pipelineCache, graphicsProgram.layout, graphicsPipeline);

    destroyShader(fragShader);
    destroyShader(vertShader);
    if (rtxSupported)
    {
        destroyShader(meshShader);
        destroyShader(taskShader);
    }
}