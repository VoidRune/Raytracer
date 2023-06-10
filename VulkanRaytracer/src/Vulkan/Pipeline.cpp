#include "Pipeline.h"

VertexAttributes::VertexAttributes(const std::vector<Attribute>& inputAttributes, uint32_t vertexSize)
{
    for (int i = 0; i < inputAttributes.size(); i++)
    {
        VkVertexInputAttributeDescription attribute;
        attribute.binding = 0;
        attribute.location = i;
        attribute.format = static_cast<VkFormat>(inputAttributes[i].format);
        attribute.offset = inputAttributes[i].offset;
        _inputAttributes.push_back(attribute);
    }
    if (vertexSize > 0)
    {
        VkVertexInputBindingDescription binding;
        binding.binding = 0;
        binding.stride = vertexSize;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        _inputBinding.push_back(binding);
    }
}

Pipeline::Pipeline(const PipelineInfo& info)
{
    _pipeline = nullptr;
    _pipelineLayout = nullptr;
    ReloadPipeline(info);
}

Pipeline::~Pipeline()
{
    vkDestroyPipelineLayout(Core::Get()->GetLogicalDevice(), _pipelineLayout, nullptr);
    vkDestroyPipeline(Core::Get()->GetLogicalDevice(), _pipeline, nullptr);
}

void Pipeline::ReloadPipeline(const PipelineInfo& info)
{
    std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();


    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(info.vertexAttributes._inputBinding.size());
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(info.vertexAttributes._inputAttributes.size());
    vertexInputInfo.pVertexBindingDescriptions = info.vertexAttributes._inputBinding.data();
    vertexInputInfo.pVertexAttributeDescriptions = info.vertexAttributes._inputAttributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = static_cast<VkPrimitiveTopology>(info.topology);
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkExtent2D extent = { 0, 0 };

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportInfo = {};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
    rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.depthClampEnable = VK_FALSE;
    rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationInfo.lineWidth = 1.0f;
    rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationInfo.depthBiasEnable = VK_FALSE;
    rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
    rasterizationInfo.depthBiasClamp = 0.0f;          // Optional
    rasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

    VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.minSampleShading = 1.0f;             // Optional
    multisampleInfo.pSampleMask = nullptr;               // Optional
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;    // Optional
    multisampleInfo.alphaToOneEnable = VK_FALSE;         // Optional

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = VK_TRUE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.minDepthBounds = 0.0f;
    depthStencilInfo.maxDepthBounds = 1.0f;
    depthStencilInfo.stencilTestEnable = VK_FALSE;
    depthStencilInfo.front = {};
    depthStencilInfo.back = {};
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachment;

    for (int i = 0; i < info.colorAttachmentsCount; i++)
    {
        VkPipelineColorBlendAttachmentState tempAttachment;
        tempAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        tempAttachment.blendEnable = VK_TRUE;
        tempAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        tempAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        tempAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        tempAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        tempAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        tempAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.push_back(tempAttachment);
    }

    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = colorBlendAttachment.size();
    colorBlendInfo.pAttachments = colorBlendAttachment.data();
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = 128;


    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &info.descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (_pipelineLayout)
    {
        vkDestroyPipelineLayout(Core::Get()->GetLogicalDevice(), _pipelineLayout, nullptr);
    }
    VkResult err = vkCreatePipelineLayout(Core::Get()->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &_pipelineLayout);
    Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to create pipeline layout!");

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = info.shaderStages.size();
    pipelineInfo.pStages = info.shaderStages.data();

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizationInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDynamicState = &dynamicState;

    pipelineInfo.layout = _pipelineLayout;// configInfo.pipelineLayout;
    pipelineInfo.renderPass = info.renderPass;
    pipelineInfo.subpass = 0;// info.subpassIndex;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (_pipeline)
    {
        vkDestroyPipeline(Core::Get()->GetLogicalDevice(), _pipeline, nullptr);
    }
    err = vkCreateGraphicsPipelines(Core::Get()->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);
    Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to create graphics pipeline!");

}

ComputePipeline::ComputePipeline(PipelineInfo info)
{
  
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &info.descriptorSetLayout;

    if (vkCreatePipelineLayout(Core::Get()->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline layout!");
    }

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.stage = info.shaderStage;

    if (vkCreateComputePipelines(Core::Get()->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }
}

ComputePipeline::~ComputePipeline()
{
    vkDestroyPipelineLayout(Core::Get()->GetLogicalDevice(), _pipelineLayout, nullptr);
    vkDestroyPipeline(Core::Get()->GetLogicalDevice(), _pipeline, nullptr);
}