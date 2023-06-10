#pragma once
#include "VKHeaders.h"

struct QueueFamilyIndices;
class Swapchain
{
public:
	Swapchain(
		VkExtent2D extent,
		uint32_t imagesCount,
		VkPresentModeKHR preferredMode);
	~Swapchain();

	uint32_t AcquireNextImage(VkSemaphore semaphore);
	void PresentImage(VkSemaphore semaphore);

	void Recreate(VkExtent2D extent);

	VkSwapchainKHR GetHandle() { return _swapchain; };
	VkFramebuffer GetFramebuffer() { return _frameBuffers[_imageIndex]; };
	VkRenderPass GetRenderPass() { return _renderPass; }

	VkSurfaceFormatKHR GetSurfaceFormat() { return _surfaceFormat; };
	VkPresentModeKHR GetPresentMode() { return _presentMode; };
	VkExtent2D GetExtent() { return _extent; };
	std::vector<VkImage>& GetImages() { return _images; }

	bool OutOfDate() { return _outOfDate; }
private:

	void CreateSwapchain(VkExtent2D extent, uint32_t imagesCount, VkPresentModeKHR preferredMode);
	void CreateImageViews();
	void CreateRenderPass();
	void CreateFramebuffers();

	VkSurfaceFormatKHR FindSurfaceFormat(VkPhysicalDevice physicalDevice);
	VkPresentModeKHR FindPresentMode(VkPhysicalDevice physicalDevice, VkPresentModeKHR preferredMode);
	VkExtent2D FindSurfaceExtent(VkPhysicalDevice physicalDevice, VkExtent2D extent);


	VkSwapchainKHR _swapchain;
	bool _outOfDate;

	VkSurfaceFormatKHR _surfaceFormat;
	VkPresentModeKHR _presentMode;
	VkExtent2D _extent;

	uint32_t _imageCount;
	uint32_t _imageIndex;

	std::vector<VkImage> _images;
	std::vector<VkImageView> _imageViews;
	std::vector<VkFramebuffer> _frameBuffers;

	VkRenderPass _renderPass;
	std::vector<VkCommandBuffer> _commandBuffers;

};