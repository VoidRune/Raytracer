#include "Framebuffer.h"

Framebuffer::Framebuffer(VkRenderPass renderpass, VkExtent2D extent, const std::vector<FramebufferAttachment>& attachments, FramebufferAttachment depthAttachment)
{
    //VkExtent2D extent = { 1024, 1024 };
    uint32_t hasDepth = (depthAttachment.format != VK_FORMAT_UNDEFINED) ? 1 : 0;
    _images.resize(attachments.size() + hasDepth);
    _imagesMemory.resize(attachments.size() + hasDepth);
    _imageViews.resize(attachments.size() + hasDepth);

    uint32_t index = 0;
    for (auto attachment : attachments)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = extent.width;
        imageInfo.extent.height = extent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = attachment.format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // Optional
        VkResult err = vkCreateImage(Core::Get()->GetLogicalDevice(), &imageInfo, nullptr, &_images[index]);
        Logger::PrintErrorIf(err != VK_SUCCESS, "Failed to create image");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(Core::Get()->GetLogicalDevice(), _images[index], &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = Core::Get()->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        err = vkAllocateMemory(Core::Get()->GetLogicalDevice(), &allocInfo, nullptr, &_imagesMemory[index]);
        Logger::PrintErrorIf(err != VK_SUCCESS, "Failed to allocate image memory!");
        vkBindImageMemory(Core::Get()->GetLogicalDevice(), _images[index], _imagesMemory[index], 0);

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = _images[index];
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = attachment.format;
        imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        err = vkCreateImageView(Core::Get()->GetLogicalDevice(), &imageViewInfo, nullptr, &_imageViews[index]);
        Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to image views!");

        index++;
    }

    if (hasDepth)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = extent.width;
        imageInfo.extent.height = extent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthAttachment.format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // Optional
        VkResult err = vkCreateImage(Core::Get()->GetLogicalDevice(), &imageInfo, nullptr, &_images[index]);
        Logger::PrintErrorIf(err != VK_SUCCESS, "Failed to create image");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(Core::Get()->GetLogicalDevice(), _images[index], &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = Core::Get()->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        err = vkAllocateMemory(Core::Get()->GetLogicalDevice(), &allocInfo, nullptr, &_imagesMemory[index]);
        Logger::PrintErrorIf(err != VK_SUCCESS, "Failed to allocate image memory!");
        vkBindImageMemory(Core::Get()->GetLogicalDevice(), _images[index], _imagesMemory[index], 0);

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = _images[index];
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = depthAttachment.format;
        imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        err = vkCreateImageView(Core::Get()->GetLogicalDevice(), &imageViewInfo, nullptr, &_imageViews[index]);
        Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to image views!");
    }

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderpass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(_imageViews.size());
    framebufferInfo.pAttachments = _imageViews.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    VkResult err = vkCreateFramebuffer(Core::Get()->GetLogicalDevice(), &framebufferInfo, nullptr, &_framebuffer);
    Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to create framebuffer!");

}

Framebuffer::~Framebuffer()
{
    for (int i = 0; i < _imagesMemory.size(); i++)
    {
        vkFreeMemory(Core::Get()->GetLogicalDevice(), _imagesMemory[i], nullptr);
    }

    for (int i = 0; i < _imageViews.size(); i++)
    {
        vkDestroyImageView(Core::Get()->GetLogicalDevice(), _imageViews[i], nullptr);
    }

    for (int i = 0; i < _images.size(); i++)
    {
        vkDestroyImage(Core::Get()->GetLogicalDevice(), _images[i], nullptr);
    }

    vkDestroyFramebuffer(Core::Get()->GetLogicalDevice(), _framebuffer, nullptr);
}