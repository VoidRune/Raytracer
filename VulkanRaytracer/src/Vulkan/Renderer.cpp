#include "Renderer.h"
#include <iostream>
#include <fstream>
#include "../../dependencies/stb/stb_image_write.h"

Renderer* Renderer::_rendererInstance = nullptr;

Renderer::Renderer(GLFWwindow* window, uint32_t imagesCount, VkPresentModeKHR preferredMode)
{
	_rendererInstance = this;

    _window = window;
	_core = std::make_unique<Core>(window);
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Core::Get()->GetPhysicalDevice(), Core::Get()->GetSurface(), &capabilities);
    
    _imageCount = std::clamp(imagesCount, capabilities.minImageCount, capabilities.maxImageCount);
    _inFlightImageCount = _imageCount - 1;

	_swapchain = std::make_unique<Swapchain>(VkExtent2D{static_cast<uint32_t>(w), static_cast<uint32_t>(h)}, _imageCount, preferredMode);
    _descriptorSetCache = std::make_unique<DescriptorSetCache>();

    CreateFrameSyncObjects();
}

void Renderer::CreateFrameSyncObjects()
{
	_frameSyncObjects.resize(_inFlightImageCount);
    _commandBuffers.resize(_inFlightImageCount);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < _frameSyncObjects.size(); i++) {
        if (vkCreateSemaphore(Core::Get()->GetLogicalDevice(), &semaphoreInfo, nullptr, &_frameSyncObjects[i].presentSemaphore) !=
            VK_SUCCESS ||
            vkCreateSemaphore(Core::Get()->GetLogicalDevice(), &semaphoreInfo, nullptr, &_frameSyncObjects[i].renderSemaphore) !=
            VK_SUCCESS ||
            vkCreateFence(Core::Get()->GetLogicalDevice(), &fenceInfo, nullptr, &_frameSyncObjects[i].renderFence) != VK_SUCCESS)
        {
            Logger::PrintFatal("Failed to create synchronization objects for a frame!");
        }
    }

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = Core::Get()->GetCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

    VkResult err = vkAllocateCommandBuffers(Core::Get()->GetLogicalDevice(), &allocInfo, _commandBuffers.data());
    Logger::PrintErrorIf(err != VK_SUCCESS, "Failed to allocate command buffers!");
}

VkCommandBuffer Renderer::BeginFrame()
{
    VkDevice device = Core::Get()->GetLogicalDevice();
    
    vkWaitForFences(
        device,
        1,
        &_frameSyncObjects[_frameIndex].renderFence,
        VK_TRUE,
        std::numeric_limits<uint64_t>::max());
    
    _swapchain->AcquireNextImage(_frameSyncObjects[_frameIndex].presentSemaphore);

    VkCommandBuffer cmd = _commandBuffers[_frameIndex];
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    VkResult err = vkBeginCommandBuffer(cmd, &beginInfo);
    Logger::PrintErrorIf(err != VK_SUCCESS, "Failed to begin recording command buffer!");
    
    renderStatistics.drawCalls = 0;
    renderStatistics.vertices = 0;
    renderStatistics.triangles = 0;

    return cmd;
}

void Renderer::EndFrame()
{
    VkCommandBuffer cmd = _commandBuffers[_frameIndex];
    VkResult err = vkEndCommandBuffer(cmd);
    Logger::PrintErrorIf(err != VK_SUCCESS, "Failed to record command buffer!");

    //if (_imagesInFlight[_imageIndex] != VK_NULL_HANDLE) {
    //    vkWaitForFences(Core::Get()->GetLogicalDevice(), 1, &_imagesInFlight[_imageIndex], VK_TRUE, UINT64_MAX);
    //}
    //_imagesInFlight[_imageIndex] = _inFlightFences[_currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { _frameSyncObjects[_frameIndex].presentSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_frameSyncObjects[_frameIndex].renderSemaphore;

    vkResetFences(Core::Get()->GetLogicalDevice(), 1, &_frameSyncObjects[_frameIndex].renderFence);
    err = vkQueueSubmit(Core::Get()->GetGraphicsQueue(), 1, &submitInfo, _frameSyncObjects[_frameIndex].renderFence);
    Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to submit command buffer!");

    _swapchain->PresentImage(_frameSyncObjects[_frameIndex].renderSemaphore);

    _frameIndex = (_frameIndex + 1) % _inFlightImageCount;
}

void Renderer::UpdateDescriptorSet(VkDescriptorSet descriptorSet, const std::vector<UpdateDescriptorSetInfo>& updateInfo)
{
    std::vector<VkWriteDescriptorSet> writeDescriptoSet(updateInfo.size());

    for (int i = 0; i < updateInfo.size(); i++)
    {
        writeDescriptoSet[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptoSet[i].dstSet = descriptorSet;
        writeDescriptoSet[i].dstBinding = updateInfo[i].binding;
        writeDescriptoSet[i].dstArrayElement = 0;
        writeDescriptoSet[i].descriptorType = static_cast<VkDescriptorType>(updateInfo[i].type);
        writeDescriptoSet[i].descriptorCount = 1;
        writeDescriptoSet[i].pBufferInfo = &updateInfo[i].bufferInfo;
        writeDescriptoSet[i].pImageInfo = &updateInfo[i].imageInfo;
        writeDescriptoSet[i].pTexelBufferView = nullptr;
    }

    vkUpdateDescriptorSets(Core::Get()->GetLogicalDevice(), static_cast<uint32_t>(writeDescriptoSet.size()), writeDescriptoSet.data(), 0, NULL);
}

VkDescriptorSet Renderer::AllocateDescriptorSet(VkDescriptorSetLayout layout)
{
    return _descriptorSetCache->AllocateDescriptorSet(layout);
}

void Renderer::CmdBindPipeline(VkCommandBuffer cmd, VkPipeline pipeline, PipelineBindPoint bindPoint)
{
    vkCmdBindPipeline(cmd, static_cast<VkPipelineBindPoint>(bindPoint), pipeline);
}

void Renderer::CmdDraw(VkCommandBuffer cmd, VkBuffer buffer, uint32_t vertexCount)
{
    VkBuffer buffers[] = { buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
    vkCmdDraw(cmd, vertexCount, 1, 0, 0);

    renderStatistics.vertices += vertexCount;
    renderStatistics.drawCalls += 1;
}

void Renderer::SaveScreenshot(const std::string& path)
{
	bool screenshotSaved = false;
	bool supportsBlit = true;

	// Check blit support for source and destination
	VkFormatProperties formatProps;

	// Check if the device supports blitting from optimal images (the swapchain images are in optimal format)
	vkGetPhysicalDeviceFormatProperties(_core->GetPhysicalDevice(), _swapchain->GetSurfaceFormat().format, &formatProps);
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		Logger::Print("Device does not support blitting from optimal tiled images, using copy instead of blit!");
		supportsBlit = false;
	}

	// Check if the device supports blitting to linear images
	vkGetPhysicalDeviceFormatProperties(_core->GetPhysicalDevice(), VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		Logger::Print("Device does not support blitting to linear tiled images, using copy instead of blit!");

		supportsBlit = false;
	}

	// Source for the copy is the last rendered swapchain image
	VkImage srcImage = _swapchain->GetImages()[_frameIndex];

	// Create the linear tiled destination image to copy to and to read the memory from
	VkImageCreateInfo imageCreateCI = {};
	imageCreateCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
	// Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color format would differ
	imageCreateCI.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateCI.extent.width = _swapchain->GetExtent().width;
	imageCreateCI.extent.height = _swapchain->GetExtent().height;
	imageCreateCI.extent.depth = 1;
	imageCreateCI.arrayLayers = 1;
	imageCreateCI.mipLevels = 1;
	imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	// Create the image
	VkImage dstImage;
	vkCreateImage(_core->GetLogicalDevice(), &imageCreateCI, nullptr, &dstImage);
	// Create memory to back up the image
	VkMemoryRequirements memRequirements;
	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkDeviceMemory dstImageMemory;
	vkGetImageMemoryRequirements(_core->GetLogicalDevice(), dstImage, &memRequirements);
	memAllocInfo.allocationSize = memRequirements.size;
	// Memory must be host visible to copy from
	memAllocInfo.memoryTypeIndex = Core::Get()->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkAllocateMemory(_core->GetLogicalDevice(), &memAllocInfo, nullptr, &dstImageMemory);
	vkBindImageMemory(_core->GetLogicalDevice(), dstImage, dstImageMemory, 0);

	
	// Do the actual blit from the swapchain image to our host visible destination image
	//VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkCommandBuffer copyCmd = _core->BeginSingleTimeCommands();
	// Transition destination image to transfer destination layout
	//vks::tools::insertImageMemoryBarrier(
	//	copyCmd,
	//	dstImage,
	//	0,
	//	VK_ACCESS_TRANSFER_WRITE_BIT,
	//	VK_IMAGE_LAYOUT_UNDEFINED,
	//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//	VK_PIPELINE_STAGE_TRANSFER_BIT,
	//	VK_PIPELINE_STAGE_TRANSFER_BIT,
	//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.image = dstImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	// Transition swapchain image from present to transfer source layout
	//vks::tools::insertImageMemoryBarrier(
	//	copyCmd,
	//	srcImage,
	//	VK_ACCESS_MEMORY_READ_BIT,
	//	VK_ACCESS_TRANSFER_READ_BIT,
	//	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	//	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	//	VK_PIPELINE_STAGE_TRANSFER_BIT,
	//	VK_PIPELINE_STAGE_TRANSFER_BIT,
	//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageMemoryBarrier.image = dstImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	// If source and destination support blit we'll blit as this also does automatic format conversion (e.g. from BGR to RGB)
	if (supportsBlit)
	{
		// Define the region to blit (we will blit the whole swapchain image)
		VkOffset3D blitSize;
		blitSize.x = _swapchain->GetExtent().width;
		blitSize.y = _swapchain->GetExtent().height;
		blitSize.z = 1;
		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = blitSize;

		// Issue the blit command
		vkCmdBlitImage(
			copyCmd,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);
	}
	else
	{
		// Otherwise use image copy (requires us to manually flip components)
		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = _swapchain->GetExtent().width;
		imageCopyRegion.extent.height = _swapchain->GetExtent().height;
		imageCopyRegion.extent.depth = 1;

		// Issue the copy command
		vkCmdCopyImage(
			copyCmd,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopyRegion);
	}

	// Transition destination image to general layout, which is the required layout for mapping the image memory later on
	//vks::tools::insertImageMemoryBarrier(
	//	copyCmd,
	//	dstImage,
	//	VK_ACCESS_TRANSFER_WRITE_BIT,
	//	VK_ACCESS_MEMORY_READ_BIT,
	//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//	VK_IMAGE_LAYOUT_GENERAL,
	//	VK_PIPELINE_STAGE_TRANSFER_BIT,
	//	VK_PIPELINE_STAGE_TRANSFER_BIT,
	//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.image = dstImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	// Transition back the swap chain image after the blit is done
	//vks::tools::insertImageMemoryBarrier(
	//	copyCmd,
	//	srcImage,
	//	VK_ACCESS_TRANSFER_READ_BIT,
	//	VK_ACCESS_MEMORY_READ_BIT,
	//	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	//	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	//	VK_PIPELINE_STAGE_TRANSFER_BIT,
	//	VK_PIPELINE_STAGE_TRANSFER_BIT,
	//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	imageMemoryBarrier.image = dstImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		copyCmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
	_core->EndSingleTimeCommands(copyCmd);
	//vulkanDevice->flushCommandBuffer(copyCmd, queue);

	// Get layout of the image (including row pitch)
	VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout(_core->GetLogicalDevice(), dstImage, &subResource, &subResourceLayout);

	// Map image memory so we can start copying from it
	uint8_t* data;
	vkMapMemory(_core->GetLogicalDevice(), dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
	data += subResourceLayout.offset;
	
	bool colorSwizzle = false;
	if (!supportsBlit)
	{
		std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
		colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), _swapchain->GetSurfaceFormat().format) != formatsBGR.end());
	}
	/*
	uint8_t* d = new uint8_t[4 * _swapchain->GetExtent().width * _swapchain->GetExtent().height];

	uint32_t index = 0;
	for (uint32_t y = 0; y < _swapchain->GetExtent().height; y++)
	{
		unsigned int* row = (unsigned int*)data;
		for (uint32_t x = 0; x < _swapchain->GetExtent().width; x++)
		{
			if (colorSwizzle)
			{
				//file.write((char*)row + 2, 1);
				//file.write((char*)row + 1, 1);
				//file.write((char*)row, 1);
				d[index + 0] = reinterpret_cast<uint8_t>(row + 2);
				d[index + 1] = reinterpret_cast<uint8_t>(row + 1);
				d[index + 2] = reinterpret_cast<uint8_t>(row + 0);
				d[index + 3] = 0xFF;

			}
			else
			{
				//file.write((char*)row, 3);
				d[index + 0] = reinterpret_cast<uint8_t>(row + 0);
				d[index + 1] = reinterpret_cast<uint8_t>(row + 1);
				d[index + 2] = reinterpret_cast<uint8_t>(row + 2);
				d[index + 3] = 0xFF;
			}
			row++;
			index += 4;
		}
		data += subResourceLayout.rowPitch;
	}
	
	std::ofstream file("test.ppm", std::ios::out | std::ios::binary);

	// ppm header
	file << "P6\n" << _swapchain->GetExtent().width << "\n" << _swapchain->GetExtent().height << "\n" << 255 << "\n";

	// If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
	// Check if source is BGR
	// Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes

	// ppm binary pixel data
	for (uint32_t y = 0; y < _swapchain->GetExtent().height; y++)
	{
		unsigned int* row = (unsigned int*)data;
		for (uint32_t x = 0; x < _swapchain->GetExtent().width; x++)
		{
			if (colorSwizzle)
			{
				file.write((char*)row + 2, 1);
				file.write((char*)row + 1, 1);
				file.write((char*)row, 1);
			}
			else
			{
				file.write((char*)row, 3);
			}
			row++;
		}
		data += subResourceLayout.rowPitch;
	}
	file.close();
	std::cout << "Screenshot saved to disk" << std::endl;
	*/
	if (colorSwizzle)
	{
		uint32_t index = 0;
		for (uint32_t y = 0; y < _swapchain->GetExtent().height; y++)
		{
			for (uint32_t x = 0; x < _swapchain->GetExtent().width; x++)
			{
				uint8_t temp = data[index + 0];
				data[index + 0] = data[index + 2];
				data[index + 2] = temp;
				
				index += 4;
			}
		}
	}

	stbi_write_png(path.c_str(), _swapchain->GetExtent().width, _swapchain->GetExtent().height, 4, data, _swapchain->GetExtent().width * 4);
	Logger::Print("Screenshot saved to disk");
	//delete[] d;
	// Clean up resources
	vkUnmapMemory(_core->GetLogicalDevice(), dstImageMemory);
	vkFreeMemory(_core->GetLogicalDevice(), dstImageMemory, nullptr);
	vkDestroyImage(_core->GetLogicalDevice(), dstImage, nullptr);

	screenshotSaved = true;
}

Renderer::~Renderer()
{
    for (int i = 0; i < _frameSyncObjects.size(); i++)
    {
        vkDestroySemaphore(Core::Get()->GetLogicalDevice(), _frameSyncObjects[i].renderSemaphore, nullptr);
        vkDestroySemaphore(Core::Get()->GetLogicalDevice(), _frameSyncObjects[i].presentSemaphore, nullptr);
        vkDestroyFence(Core::Get()->GetLogicalDevice(), _frameSyncObjects[i].renderFence, nullptr);
    }
}