#pragma once
#include "VKHeaders.h"

class DescriptorSetCache
{
public:
	DescriptorSetCache();
	~DescriptorSetCache();

	VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout);

	VkDescriptorPool GetHandle() { return _descriptorPool; }

private:

	VkDescriptorPool _descriptorPool;
};

class DescriptorSetLayout
{
public:
	struct DescriptorSetInfo
	{
		uint32_t count;
		DescriptorType type;
		ShaderStage shaderStage;
	};

	DescriptorSetLayout(const std::vector<DescriptorSetInfo>& descriptorSetInfo);
	~DescriptorSetLayout();

	VkDescriptorSetLayout GetHandle() { return _layout; };
private:
	VkDescriptorSetLayout _layout;
};