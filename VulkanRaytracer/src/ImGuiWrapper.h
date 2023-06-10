#pragma once

#include "../dependencies/imgui-master/imgui.h"
#include "../dependencies/imgui-master/imgui_impl_glfw.h"
#include "../dependencies/imgui-master/imgui_impl_vulkan.h"

void ImGuiInit(GLFWwindow* window);
void ImGuiShutdown();

void ImGuiBeginFrame();
void ImGuiEndFrame(VkCommandBuffer cmd);