#pragma once
#include "Core.h"
#include "Swapchain.h"
#include "DescriptorSetCache.h"

class Core;
class Swapchain;
class DescriptorSetCache;
class Renderer
{
public:
	Renderer(GLFWwindow* window, uint32_t imagesCount, VkPresentModeKHR preferredMode);
	~Renderer();

	VkCommandBuffer BeginFrame();
	void EndFrame();

	Swapchain* GetSwapchain() { return _swapchain.get(); }
	DescriptorSetCache* GetDescriptorSetCache() { return _descriptorSetCache.get(); }

	uint32_t GetFrameIndex() { return _frameIndex; }
	uint32_t GetImageCount() { return _imageCount; }
	uint32_t GetInFlightImageCount() { return _inFlightImageCount; }

	struct UpdateDescriptorSetInfo
	{
		uint32_t binding;
		DescriptorType type;
		VkDescriptorBufferInfo bufferInfo;
		VkDescriptorImageInfo imageInfo;
	};
	void UpdateDescriptorSet(VkDescriptorSet descriptorSet, const std::vector<UpdateDescriptorSetInfo>& updateInfo);
	VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout);

	struct RenderStatistics
	{
		uint32_t drawCalls;
		uint32_t vertices;
		uint32_t triangles;
	} renderStatistics;
	void CmdBindPipeline(VkCommandBuffer cmd, VkPipeline pipeline, PipelineBindPoint bindPoint);
	void CmdDraw(VkCommandBuffer cmd, VkBuffer buffer, uint32_t vertexCount);

	void SaveScreenshot(const std::string& path);

	static Renderer* Get() { return _rendererInstance; }
private:

	void CreateFrameSyncObjects();

	GLFWwindow* _window;
	std::unique_ptr<Core> _core;
	std::unique_ptr<Swapchain> _swapchain;
	std::unique_ptr<DescriptorSetCache> _descriptorSetCache;

	struct FrameSyncObjects
	{
		VkSemaphore	presentSemaphore;
		VkSemaphore	renderSemaphore;
		VkFence	renderFence;
	};
	std::vector<FrameSyncObjects> _frameSyncObjects;
	std::vector<VkCommandBuffer> _commandBuffers;
	uint32_t _frameIndex;
	uint32_t _imageCount;
	uint32_t _inFlightImageCount;

	static Renderer* _rendererInstance;
};