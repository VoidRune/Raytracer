#pragma once
#include "VKHeaders.h"

class RenderPass
{
public:
    struct AttachmentDesc;
    RenderPass(const std::vector<AttachmentDesc>& colorAttachments, AttachmentDesc depthAttachment);
    ~RenderPass();

    struct AttachmentDesc
    {
        VkFormat format;
        VkAttachmentLoadOp loadOp;
    };

    VkRenderPass GetHandle() { return _renderPass; };
    std::vector<AttachmentDesc>& GetColorAttachments() { return colorAttachmentDescs; }
    AttachmentDesc GetDepthAttachment() { return depthAttachmentDesc; }
private:

    VkRenderPass _renderPass;
    std::vector<AttachmentDesc> colorAttachmentDescs;
    AttachmentDesc depthAttachmentDesc;
};