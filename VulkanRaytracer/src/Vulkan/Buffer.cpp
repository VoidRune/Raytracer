#include "Buffer.h"

Buffer::Buffer(uint32_t size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryVisibility)
{
	_size = size;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult err = vkCreateBuffer(Core::Get()->GetLogicalDevice(), &bufferInfo, nullptr, &_buffer);
    Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to create buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(Core::Get()->GetLogicalDevice(), _buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Core::Get()->FindMemoryType(memRequirements.memoryTypeBits, memoryVisibility);

    err = vkAllocateMemory(Core::Get()->GetLogicalDevice(), &allocInfo, nullptr, &_bufferMemory);
    Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to allocate buffer memory!");

    vkBindBufferMemory(Core::Get()->GetLogicalDevice(), _buffer, _bufferMemory, 0);
}

Buffer::~Buffer()
{
    vkFreeMemory(Core::Get()->GetLogicalDevice(), _bufferMemory, nullptr);
    vkDestroyBuffer(Core::Get()->GetLogicalDevice(), _buffer, nullptr);
    _buffer = nullptr;
}

void* Buffer::Map()
{
    void* data;
    vkMapMemory(Core::Get()->GetLogicalDevice(), _bufferMemory, 0, _size, 0, &data);
    return data;
}

void Buffer::Unmap()
{
    vkUnmapMemory(Core::Get()->GetLogicalDevice(), _bufferMemory);
}