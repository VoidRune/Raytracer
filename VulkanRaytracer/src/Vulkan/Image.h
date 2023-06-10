#pragma once
#include "VKHeaders.h"

class Image
{
public:
	Image(VkExtent2D extent, Format format, VkImageUsageFlags usageFlags);
	~Image();
	void SetData(const void* data, uint32_t size, ImageLayout newLayout);

	uint32_t Width() { return _width; }
	uint32_t Height() { return _height; }

	VkImage GetHandle() { return _image; }
	VkDeviceMemory GetDeviceMemory() { return _imageMemory; }
	VkImageView GetImageView() { return _imageView; }
	VkImageLayout GetImageLayout() { return _layout; }

private:

	uint32_t _width;
	uint32_t _height;

	VkImage _image;
	VkDeviceMemory _imageMemory;
	VkImageView _imageView;
	VkImageLayout _layout;
};

class Sampler
{
public:
	Sampler(Filter magFilter, Filter minFilter);
	~Sampler();

	VkSampler GetHandle() { return _sampler; }
private:
	VkSampler _sampler;
};