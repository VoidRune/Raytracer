#include "RenderPass.h"

RenderPass::RenderPass(const std::vector<AttachmentDesc>& colorAttachments, AttachmentDesc depthAttachment)
{
    colorAttachmentDescs = colorAttachments;
    depthAttachmentDesc = depthAttachment;

    uint32_t currAttachmentIndex = 0;

    std::vector<VkAttachmentDescription> attachmentsDesc;
    std::vector<VkAttachmentReference> colorRef;

    for (auto colorAttachmentDesc : colorAttachments)
    {
        VkAttachmentDescription attachmentDesc = {};
        attachmentDesc.format = colorAttachmentDesc.format;
        attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDesc.loadOp = colorAttachmentDesc.loadOp;
        attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachmentsDesc.push_back(attachmentDesc);

        VkAttachmentReference attachmentRef = {};
        attachmentRef.attachment = currAttachmentIndex++;
        attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorRef.push_back(attachmentRef);
    }

    std::array<VkSubpassDescription, 1> subpass = {};
    subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass[0].colorAttachmentCount = colorRef.size();
    subpass[0].pColorAttachments = colorRef.data();

    //subpass[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    //subpass[1].colorAttachmentCount = colorRef.size();
    //subpass[1].pColorAttachments = colorRef.data();

    VkAttachmentReference depthRef = {};

    if (depthAttachment.format != VK_FORMAT_UNDEFINED)
    {
        VkAttachmentDescription depthAttachmentTemp = {};
        depthAttachmentTemp.format = depthAttachment.format;
        depthAttachmentTemp.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachmentTemp.loadOp = depthAttachment.loadOp;
        depthAttachmentTemp.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachmentTemp.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentTemp.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentTemp.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachmentTemp.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        attachmentsDesc.push_back(depthAttachmentTemp);

        depthRef.attachment = currAttachmentIndex++;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpass[0].pDepthStencilAttachment = &depthRef;
        //subpass[1].pDepthStencilAttachment = &depthRef;
    }

    std::array<VkSubpassDependency, 1> dependency = {};
    dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency[0].dstSubpass = 0;
    dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency[0].srcAccessMask = 0;
    dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    //dependency[1].srcSubpass = 0;
    //dependency[1].dstSubpass = 1;
    //dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    //dependency[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    //dependency[1].srcAccessMask = 0;
    //dependency[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachmentsDesc.size();
    renderPassInfo.pAttachments = attachmentsDesc.data();
    renderPassInfo.subpassCount = subpass.size();
    renderPassInfo.pSubpasses = subpass.data();
    renderPassInfo.dependencyCount = dependency.size();
    renderPassInfo.pDependencies = dependency.data();

    VkResult err = vkCreateRenderPass(Core::Get()->GetLogicalDevice(), &renderPassInfo, nullptr, &_renderPass);
    Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to create render pass!");
}

RenderPass::~RenderPass()
{
    vkDestroyRenderPass(Core::Get()->GetLogicalDevice(), _renderPass, nullptr);
}