#include "Shader.h"
#include "SpirvCompiler.h"
#include <fstream>
#include <iostream>
#include <mutex>

Shader::Shader(const std::string& filename)
{
	_shaderModule = nullptr;
	Reload(filename);
}

Shader::~Shader()
{
	vkDestroyShaderModule(Core::Get()->GetLogicalDevice(), _shaderModule, nullptr);
}

void Shader::Reload(const std::string& filename)
{
	//std::vector<uint32_t> bytecode = GetBytecode(filename);
	//auto shaderModuleCreateInfo = vk::ShaderModuleCreateInfo()
	//    .setCodeSize(bytecode.size() * sizeof(uint32_t))
	//    .setPCode(bytecode.data());
	//_shaderModule = logicalDevice.createShaderModuleUnique(shaderModuleCreateInfo);
	//spirv_cross::Compiler compiler(bytecode.data(), bytecode.size());

	std::string result;
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (in)
	{
		in.seekg(0, std::ios::end);
		result.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&result[0], result.size());
		in.close();
	}
	else
	{
		std::cout << "Coul not open file: " << filename << std::endl;
	}
	std::filesystem::path path = filename;
	std::string extension = path.extension().string();

	VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
	if (extension == ".vert")
		shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
	if (extension == ".frag")
		shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
	if (extension == ".comp")
		shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;

	//SpirvHelper::Init();
	std::vector<uint32_t> SpirV;
	bool success = SpirvHelper::GLSLtoSPV(shaderStage, result.c_str(), SpirV);
	//SpirvHelper::Finalize();

	//spirv_cross::Compiler compiler(SpirV);
	//spirv_cross::ShaderResources resources = compiler.get_shader_resources();
	//
	//for (const auto& resource : resources.uniform_buffers)
	//{
	//	const auto& bufferType = compiler.get_type(resource.base_type_id);
	//	uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
	//	uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
	//
	//}

	if (success == true)
	{
		if (_shaderModule)
		{
			vkDestroyShaderModule(Core::Get()->GetLogicalDevice(), _shaderModule, nullptr);
			_shaderModule = nullptr;
		}

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = SpirV.size() * sizeof(uint32_t);
		createInfo.pCode = reinterpret_cast<const uint32_t*>(SpirV.data());

		VkResult err = vkCreateShaderModule(Core::Get()->GetLogicalDevice(), &createInfo, nullptr, &_shaderModule);
		Logger::PrintErrorIf(err != VK_SUCCESS, "Failed to create shader module");


		_shaderStage = {};
		_shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		_shaderStage.stage = shaderStage;
		_shaderStage.module = _shaderModule;
		_shaderStage.pName = "main";
		_shaderStage.flags = 0;
		_shaderStage.pNext = nullptr;
		_shaderStage.pSpecializationInfo = nullptr;
	}
}

std::vector<uint32_t>& Shader::GetBytecode(std::string filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("failed to open file!");

    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> bytecode(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read((char*)bytecode.data(), bytecode.size() * sizeof(uint32_t));
    file.close();
    return bytecode;
}