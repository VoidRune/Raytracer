#include "DescriptorSetCache.h"

DescriptorSetCache::DescriptorSetCache()
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	VkDescriptorPoolSize uniformPoolSize;
	uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	uniformPoolSize.descriptorCount = 1000;
	poolSizes.push_back(uniformPoolSize);
	VkDescriptorPoolSize imageSamplerPoolSize;
	imageSamplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageSamplerPoolSize.descriptorCount = 1000;
	poolSizes.push_back(imageSamplerPoolSize);


	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = 1000;
	descriptorPoolInfo.flags = 0;

	VkResult err = vkCreateDescriptorPool(Core::Get()->GetLogicalDevice(), &descriptorPoolInfo, nullptr, &_descriptorPool);
	Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to create descriptor pool!");
}

DescriptorSetCache::~DescriptorSetCache()
{
	vkDestroyDescriptorPool(Core::Get()->GetLogicalDevice(), _descriptorPool, nullptr);
}

DescriptorSetLayout::DescriptorSetLayout(const std::vector<DescriptorSetInfo>& descriptorSetInfo)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	for (int i = 0; i < descriptorSetInfo.size(); i++)
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = i;
		binding.descriptorCount = descriptorSetInfo[i].count;
		binding.descriptorType = static_cast<VkDescriptorType>(descriptorSetInfo[i].type);
		binding.stageFlags = static_cast<VkShaderStageFlags>(descriptorSetInfo[i].shaderStage);
		binding.pImmutableSamplers = nullptr;
		bindings.push_back(binding);
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorSetLayoutInfo.pBindings = bindings.data();

	VkResult err = vkCreateDescriptorSetLayout(
		Core::Get()->GetLogicalDevice(),
		&descriptorSetLayoutInfo,
		nullptr,
		&_layout);
	Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to create descriptor set layout!");
}

DescriptorSetLayout::~DescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(Core::Get()->GetLogicalDevice(), _layout, nullptr);
}

VkDescriptorSet DescriptorSetCache::AllocateDescriptorSet(VkDescriptorSetLayout layout)
{
	VkDescriptorSetAllocateInfo descAllocInfo{};
	descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descAllocInfo.descriptorPool = _descriptorPool;
	descAllocInfo.descriptorSetCount = 1;
	descAllocInfo.pSetLayouts = &layout;

	VkDescriptorSet descriptorSet;
	VkResult err = vkAllocateDescriptorSets(Core::Get()->GetLogicalDevice(), &descAllocInfo, &descriptorSet);
	Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to allocate descriptor set");
	return descriptorSet;
}

//FrameDescriptor::FrameDescriptor(VkDescriptorSetLayout layout)
//{
//	_descriptorSets.resize(Renderer::Get()->GetInFlightImageCount());
//
//	VkDescriptorSetAllocateInfo descAllocInfo{};
//	descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//	descAllocInfo.descriptorPool = _descriptorPool;
//	descAllocInfo.descriptorSetCount = Renderer::Get()->GetInFlightImageCount();
//	descAllocInfo.pSetLayouts = &layout;
//
//	VkResult err = vkAllocateDescriptorSets(Core::Get()->GetLogicalDevice(), &descAllocInfo, _descriptorSets.data());
//	Logger::PrintFatalIf(err != VK_SUCCESS, "Failed to allocate descriptor set");
//}
//
//FrameDescriptor::~FrameDescriptor()
//{
//
//}