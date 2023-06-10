#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX

#include <vulkan/vulkan.h>

#include "GLFW/glfw3.h"

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "glm/gtx/hash.hpp"
#ifdef _DEBUG
#define VALIDATION
#endif

#include "../Logger.h"
#include <vector>
#include <array>
#include <memory>
#include <filesystem>
#include <random>

#include "Common.h"
#include "Core.h"
#include "Swapchain.h"
#include "Renderer.h"
#include "Shader.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "Buffer.h"
#include "Image.h"
#include "DescriptorSetCache.h"
#include "SpirvCompiler.h"
