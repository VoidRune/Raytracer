#pragma once
#include "VKHeaders.h"

struct QueueFamilyIndices
{
	uint32_t graphicsFamilyIndex;
	uint32_t presentFamilyIndex;
};

class Swapchain;
class Core
{
public:
	Core(GLFWwindow* window);
	~Core();

	void WaitIdle();

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	VkInstance GetInstance() { return _instance; }
	VkDevice GetLogicalDevice() { return _logicalDevice; }
	VkPhysicalDevice GetPhysicalDevice() { return _physicalDevice; }
	VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() { return _physicalDeviceProperties; }
	VkCommandPool GetCommandPool() { return _commandPool; }

	QueueFamilyIndices GetQueueIndices() { return _queueFamilyIndices; }
	VkQueue GetGraphicsQueue() { return _graphicsQueue; }
	VkQueue GetPresentQueue() { return _presentQueue; }
	VkSurfaceKHR GetSurface() { return _surface; }

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryVisibility);

	static Core* Get() { return _coreInstance; }
private:

	void CreateInstance();
	void CreateDebugUtilsMessenger();
	void CreateSurface(GLFWwindow* window);
	void FindPhysicalDevice();
	void FindQueueFamilyIndices();
	void CreateLogicalDevice();
	void GetDeviceQueue();
	void CreateCommandPool();

	VkInstance _instance;
	VkSurfaceKHR _surface;
	VkPhysicalDevice _physicalDevice;
	VkPhysicalDeviceProperties _physicalDeviceProperties;
	VkDevice _logicalDevice;
	VkCommandPool _commandPool;

	QueueFamilyIndices _queueFamilyIndices;
	VkQueue _graphicsQueue;
	VkQueue _presentQueue;

#ifdef VALIDATION
	VkDebugUtilsMessengerEXT	_debugMessenger;
#endif

	static Core* _coreInstance;
};
