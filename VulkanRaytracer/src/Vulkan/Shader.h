#pragma once
#include "VKHeaders.h"

class Shader
{
public:
    Shader(const std::string& filename);
    ~Shader();

    void Reload(const std::string& filename);

    VkShaderModule GetShaderModule() { return _shaderModule; };
    VkPipelineShaderStageCreateInfo GetShaderStage() { return _shaderStage; }
private:

    std::vector<uint32_t>& GetBytecode(std::string filename);

    VkShaderModule _shaderModule;
    VkPipelineShaderStageCreateInfo _shaderStage;
};