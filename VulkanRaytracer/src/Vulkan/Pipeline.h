#pragma once
#include "VKHeaders.h"

class VertexAttributes
{
public:
	struct Attribute
	{
		Format format;
		uint32_t offset;
	};
	VertexAttributes(const std::vector<Attribute>& inputAttributes, uint32_t vertexSize);
	~VertexAttributes() {};

	std::vector<VkVertexInputBindingDescription> _inputBinding;
	std::vector<VkVertexInputAttributeDescription> _inputAttributes;
};

class Core;
class Pipeline
{
public:
	struct PipelineInfo;

	Pipeline(const PipelineInfo& info);
	~Pipeline();

	void ReloadPipeline(const PipelineInfo& info);

	VkPipeline GetHandle() { return _pipeline; }
	VkPipelineLayout GetLayout() { return _pipelineLayout; }

	struct PipelineInfo
	{
		const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages;
		PrimitiveTopology topology;
		VkRenderPass renderPass;
		VertexAttributes vertexAttributes;
		VkDescriptorSetLayout descriptorSetLayout;
		uint32_t colorAttachmentsCount;
		//uint32_t subpassIndex;
	};

private:

	VkPipeline _pipeline;
	VkPipelineLayout _pipelineLayout;
};

class ComputePipeline
{
public:
	struct PipelineInfo;

	ComputePipeline(PipelineInfo info);
	~ComputePipeline();

	VkPipeline GetHandle() { return _pipeline; }
	VkPipelineLayout GetLayout() { return _pipelineLayout; }

	struct PipelineInfo
	{
		VkPipelineShaderStageCreateInfo shaderStage;
		VkDescriptorSetLayout descriptorSetLayout;
		VkRenderPass renderPass;
	};

private:

	VkPipeline _pipeline;
	VkPipelineLayout _pipelineLayout;
	VkDescriptorSetLayout _setLayout;
};