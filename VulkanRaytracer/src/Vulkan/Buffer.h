#pragma once
#include "VKHeaders.h"

class Buffer
{
public:
	Buffer(uint32_t size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryVisibility);
	~Buffer();

    void* Map();
    void Unmap();

	VkBuffer GetHandle() { return _buffer; };
	VkDeviceMemory GetMemory() { return _bufferMemory; }

private:

	VkBuffer _buffer;
	VkDeviceMemory _bufferMemory;
	uint32_t _size;

};