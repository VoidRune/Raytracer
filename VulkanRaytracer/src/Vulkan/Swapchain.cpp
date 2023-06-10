#include "Swapchain.h"
#include <algorithm>

Swapchain::Swapchain(VkExtent2D extent, uint32_t imagesCount, VkPresentModeKHR preferredMode)
{
    _swapchain = VK_NULL_HANDLE;
    CreateSwapchain(extent, imagesCount, preferredMode);
    CreateImageViews();
    CreateRenderPass();
    CreateFramebuffers();
    _outOfDate = false;
}

void Swapchain::Recreate(VkExtent2D extent)
{

    Core::Get()->WaitIdle();

    for (size_t i = 0; i < _frameBuffers.size(); i++)
        vkDestroyFramebuffer(Core::Get()->GetLogicalDevice(), _frameBuffers[i], nullptr);
    for (size_t i = 0; i < _imageViews.size(); i++)
        vkDestroyImageView(Core::Get()->GetLogicalDevice(), _imageViews[i], nullptr);
    _imageViews.clear();

    CreateSwapchain(extent, _imageCount, _presentMode);
    CreateImageViews();
    //CreateRenderPass();
    CreateFramebuffers();
    _outOfDate = false;
}

void Swapchain::CreateSwapchain(VkExtent2D extent, uint32_t imagesCount, VkPresentModeKHR preferredMode)
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Core::Get()->GetPhysicalDevice(), Core::Get()->GetSurface(), &capabilities);

    _surfaceFormat = FindSurfaceFormat(Core::Get()->GetPhysicalDevice());
    _presentMode = FindPresentMode(Core::Get()->GetPhysicalDevice(), preferredMode);
    _extent = FindSurfaceExtent(Core::Get()->GetPhysicalDevice(), extent);

    _imageCount = imagesCount;
    _imageIndex = 0;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = Core::Get()->GetSurface();
    createInfo.minImageCount = _imageCount;
    createInfo.imageFormat = _surfaceFormat.format;
    createInfo.imageColorSpace = _surfaceFormat.colorSpace;
    createInfo.imageExtent = _extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    uint32_t indices[] = { Core::Get()->GetQueueIndices().graphicsFamilyIndex, Core::Get()->GetQueueIndices().presentFamilyIndex };
    if (indices[0] != indices[1])
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = indices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = _presentMode;
    createInfo.clipped = VK_TRUE;
    VkSwapchainKHR oldSwapchain = _swapchain;
    createInfo.oldSwapchain = oldSwapchain;

    VkResult err = vkCreateSwapchainKHR(Core::Get()->GetLogicalDevice(), &createInfo, nullptr, &_swapchain);
    Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to create swap chain!");

    if(oldSwapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(Core::Get()->GetLogicalDevice(), oldSwapchain, nullptr);
}

void Swapchain::CreateImageViews()
{
    vkGetSwapchainImagesKHR(Core::Get()->GetLogicalDevice(), _swapchain, &_imageCount, nullptr);
    _images.resize(_imageCount);
    vkGetSwapchainImagesKHR(Core::Get()->GetLogicalDevice(), _swapchain, &_imageCount, _images.data());

    _imageViews.resize(_images.size());

    for (size_t i = 0; i < _images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = _images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = _surfaceFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult err = vkCreateImageView(Core::Get()->GetLogicalDevice(), &createInfo, nullptr, &_imageViews[i]);
        Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to image views!");
    }
}

void Swapchain::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = _surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstSubpass = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 1> attachments = { colorAttachment };
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult err = vkCreateRenderPass(Core::Get()->GetLogicalDevice(), &renderPassInfo, nullptr, &_renderPass);
    Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to create render pass!");
}

void Swapchain::CreateFramebuffers()
{
    _frameBuffers.resize(_imageCount);
    for (size_t i = 0; i < _frameBuffers.size(); i++) {
        std::array<VkImageView, 1> attachments = { _imageViews[i] };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _extent.width;
        framebufferInfo.height = _extent.height;
        framebufferInfo.layers = 1;

        VkResult err = vkCreateFramebuffer(Core::Get()->GetLogicalDevice(), &framebufferInfo, nullptr, &_frameBuffers[i]);
        Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to create framebuffer!");

    }
}

Swapchain::~Swapchain()
{
    vkDestroyRenderPass(Core::Get()->GetLogicalDevice(), _renderPass, nullptr);

    for (size_t i = 0; i < _frameBuffers.size(); i++)
    {
        vkDestroyFramebuffer(Core::Get()->GetLogicalDevice(), _frameBuffers[i], nullptr);
    }

    for (size_t i = 0; i < _imageViews.size(); i++)
    {
        vkDestroyImageView(Core::Get()->GetLogicalDevice(), _imageViews[i], nullptr);
    }
    _imageViews.clear();

    vkDestroySwapchainKHR(Core::Get()->GetLogicalDevice(), _swapchain, nullptr);
}

uint32_t Swapchain::AcquireNextImage(VkSemaphore semaphore)
{
    VkResult err = vkAcquireNextImageKHR(
        Core::Get()->GetLogicalDevice(),
        _swapchain,
        std::numeric_limits<uint64_t>::max(),
        semaphore,
        VK_NULL_HANDLE,
        &_imageIndex);

    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        //Core::Get()->WaitIdle();
        _outOfDate = true;
        //Destroy();
        //Create(imagesCount, preferredMode);
    }

    return _imageIndex;
}

void Swapchain::PresentImage(VkSemaphore semaphore)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &_imageIndex;

    VkResult err = vkQueuePresentKHR(Core::Get()->GetPresentQueue(), &presentInfo);

    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        //Core::Get()->WaitIdle();
        _outOfDate = 1;
    }
    else
    {
        Logger::PrintErrorIf(err != VK_SUCCESS, "Failed to present swap chain image!");
    }
}

VkSurfaceFormatKHR Swapchain::FindSurfaceFormat(VkPhysicalDevice physicalDevice)
{
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, Core::Get()->GetSurface(), &formatCount, nullptr);

    std::vector<VkSurfaceFormatKHR> formats;
    if (formatCount != 0)
    {
        formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, Core::Get()->GetSurface(), &formatCount, formats.data());
    }

    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        VkSurfaceFormatKHR format;
        format.format = VK_FORMAT_B8G8R8A8_UNORM;
        format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        return format;
    }

    for (VkSurfaceFormatKHR& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return formats[0];
}

VkPresentModeKHR Swapchain::FindPresentMode(VkPhysicalDevice physicalDevice, VkPresentModeKHR preferredMode)
{
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, Core::Get()->GetSurface(), &presentModeCount, nullptr);

    std::vector<VkPresentModeKHR> presentModes;
    if (presentModeCount != 0)
    {
        presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, Core::Get()->GetSurface(), &presentModeCount, presentModes.data());
    }

    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == preferredMode) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::FindSurfaceExtent(VkPhysicalDevice physicalDevice, VkExtent2D extent)
{
    VkExtent2D result;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, Core::Get()->GetSurface(), &capabilities);

    if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
    {
        result = capabilities.currentExtent;
    }
    else {

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(extent.width),
            static_cast<uint32_t>(extent.height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        result = actualExtent;
    }

    return result;
}