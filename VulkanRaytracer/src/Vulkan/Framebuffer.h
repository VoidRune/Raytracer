#pragma once
#include "VKHeaders.h"

class Framebuffer
{
public:
	struct FramebufferAttachment;
	Framebuffer(VkRenderPass renderpass, VkExtent2D extent, const std::vector<FramebufferAttachment>& attachments, FramebufferAttachment depthAttachment);
	~Framebuffer();

	VkFramebuffer GetHandle() { return _framebuffer; }
	std::vector<VkImageView>& GetImageView() { return _imageViews; }

	struct FramebufferAttachment
	{
		VkFormat format;
	};
private:

	VkFramebuffer _framebuffer;

	std::vector<VkImage> _images;
	std::vector<VkDeviceMemory> _imagesMemory;
	std::vector<VkImageView> _imageViews;
};