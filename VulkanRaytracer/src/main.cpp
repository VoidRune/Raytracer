#include "Vulkan/VKHeaders.h"
#include <iostream>
#include "Camera/CameraFPS.h"
#include <future>
#include "Audio/AudioEngine.h"
#include "ModelLoader.h"
#include "Helper.h"
#include "ImGuiWrapper.h"
#include "../dependencies/stb/stb_image_write.h"

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    VkExtent2D windowExtent = { 1280, 720 };
	GLFWwindow* window = glfwCreateWindow(windowExtent.width, windowExtent.height, "Vulkan raytracer", nullptr, nullptr);
    //VkExtent2D windowExtent = { 1920, 1080 };
    //GLFWwindow* window = glfwCreateWindow(windowExtent.width, windowExtent.height, "Vulkan engine", glfwGetPrimaryMonitor(), nullptr);

    std::unique_ptr<Renderer> renderer = std::make_unique<Renderer>(window, 2, VK_PRESENT_MODE_FIFO_KHR);

    ImGuiInit(window);

    {
        std::unique_ptr<Shader> presentVert;
        std::unique_ptr<Shader> presentFrag;
        std::unique_ptr<Shader> computeShader;
        SpirvHelper::Init();
        auto fut1 = std::async(std::launch::async, [&]() {
            presentVert = std::make_unique<Shader>("res/Shaders/present.vert");
            });
        auto fut2 = std::async(std::launch::async, [&]() {
            presentFrag = std::make_unique<Shader>("res/Shaders/present.frag");
            });
        auto fut3 = std::async(std::launch::async, [&]() {
            computeShader = std::make_unique<Shader>("res/Shaders/Raytracing.comp");
            });
        fut1.get();
        fut2.get();
        fut3.get();
        SpirvHelper::Finalize();

        RenderPass renderPass(
            {
                { VK_FORMAT_R32G32B32A32_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR },
                { VK_FORMAT_B8G8R8A8_SRGB, VK_ATTACHMENT_LOAD_OP_CLEAR }
            },
            { VK_FORMAT_D32_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR /*VK_FORMAT_UNDEFINED*/ }
        );
        //VK_FORMAT_R32G32B32A32_SFLOAT

        DescriptorSetLayout layout({
            { 1, DescriptorType::CombinedImageSampler, ShaderStage::Fragment }
            });

        VkDescriptorSet descriptor = renderer->AllocateDescriptorSet(layout.GetHandle());

        VertexAttributes emptyVertexAttributes({ }, 0);

        Pipeline presentPipeline(
            {
                {
                    presentVert->GetShaderStage(),
                    presentFrag->GetShaderStage()
                },
                PrimitiveTopology::TriangleList,
                renderer->GetSwapchain()->GetRenderPass(),
                emptyVertexAttributes,
                layout.GetHandle(),
                1
            });

        Sampler linearSampler(Filter::Linear, Filter::Linear);
        Image raytracingImage(windowExtent, Format::R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        Image accumulationImage(windowExtent, Format::R32G32B32A32_Sfloat, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        //void* d = LoadImageFromFile("res/Image/test.jpg");
        uint32_t* d = new uint32_t[4 * windowExtent.width * windowExtent.height];
        memset(d, 0, 4 * windowExtent.width * windowExtent.height);
        raytracingImage.SetData(d, 4 * windowExtent.width * windowExtent.height, ImageLayout::General);
        accumulationImage.SetData(d, 4 * 4 * windowExtent.width * windowExtent.height, ImageLayout::General);
        delete[] d;
        //ReleaseImageData(d);

        renderer->UpdateDescriptorSet(descriptor, {
            { 0, DescriptorType::CombinedImageSampler, {}, {linearSampler.GetHandle(), raytracingImage.GetImageView(), VK_IMAGE_LAYOUT_GENERAL }}
            });

        DescriptorSetLayout computeLayout({
            { 1, DescriptorType::UniformBuffer, ShaderStage::Compute },
            { 1, DescriptorType::StorageBuffer, ShaderStage::Compute },
            { 1, DescriptorType::StorageBuffer, ShaderStage::Compute },
            { 1, DescriptorType::StorageBuffer, ShaderStage::Compute },
            { 1, DescriptorType::StorageImage, ShaderStage::Compute },
            { 1, DescriptorType::StorageImage, ShaderStage::Compute },
            });

        VkDescriptorSet computeDescriptor = renderer->AllocateDescriptorSet(computeLayout.GetHandle());
        ComputePipeline computePipeline({ 
            computeShader->GetShaderStage(),
            computeLayout.GetHandle(),
            renderPass.GetHandle()
            });

        FrameData frameData;
        glm::vec3 position = { 4.0 * cos(0.0), 1.5, 4.0 * sin(0.0) };
        glm::vec3 forward = glm::normalize(-position);
        frameData.cameraInverseProjection = glm::inverse(glm::perspectiveFov(70.0f, (float)windowExtent.width, (float)windowExtent.height, 0.1f, 1000.0f));
        //frameData.cameraInverseView = glm::inverse(glm::lookAtLH({ 0, 0, 0 }, glm::vec3{ 4.0, 1.5, 0.0 }, { 0.0, 1.0, 0.0 }));
        frameData.cameraInverseView = glm::inverse(glm::lookAtLH({ 0, 0, 0 }, position, { 0.0, 1.0, 0.0 }));
        frameData.cameraPos = glm::vec4(position.x, position.y, position.z, 0);
        //frameData.cameraPos = glm::vec4( 4.0, 1.5, 0.0, 0.0 );
        frameData.cameraDirection = glm::vec4(forward.x, forward.y, forward.z, 0);
        //frameData.cameraDirection = glm::normalize(-frameData.cameraPos);

        frameData.window.x = windowExtent.width;
        frameData.window.y = windowExtent.height;
        frameData.raysPerPixel = 4;
        frameData.maxBouceLimit = 6;
        frameData.frameIndex = 0;
        frameData.sunLightDirection = glm::vec4(-0.4, -0.4, -0.4, 0.0);
        frameData.sunFocus = 1.0f;
        frameData.sunIntensity = 1.0f;

        std::vector<glm::vec4> skyHorizonColors =
        {
            {0.7, 0.3, 0.1, 0.0},
            {0.93, 0.227, 0.177, 0.0},
            {0.0, 0.0, 0.0, 0.0},
            {0.93, 0.227, 0.177, 0.0},
            {0.7, 0.3, 0.1, 0.0},
        };
        std::vector<glm::vec4> skyZenithColors =
        {
            {0.2, 0.56, 0.95, 0.0},
            {0.0, 0.0, 0.0, 0.0},
            {0.2, 0.56, 0.95, 0.0},
        };
        std::vector<glm::vec4> groundColors =
        {
            {1.0, 1.0, 1.0, 0.0},
            {0.3, 0.3, 0.3, 0.0},
            {0.0, 0.0, 0.0, 0.0},
            {0.3, 0.3, 0.3, 0.0},
            {1.0, 1.0, 1.0, 0.0}
        };

        //frameData.skyColorHorizon = LerpM(skyHorizonColors, 0.0);
        //frameData.skyColorZenith = LerpM(skyZenithColors, 0.0);
        //frameData.groundColor = LerpM(groundColors, 0.0);
        frameData.skyColorHorizon = { 0.7, 0.3, 0.1, 0.0 };
        frameData.skyColorZenith = { 0.2, 0.56, 0.95, 0.0 };
        frameData.groundColor = { 0.9, 0.9, 0.9, 0.0 };

        Buffer computeFrameBuffer(sizeof(FrameData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        void* copyData = computeFrameBuffer.Map();
        memcpy(copyData, &frameData, static_cast<size_t>(sizeof(FrameData)));
        computeFrameBuffer.Unmap();

        
        std::vector<Sphere> spheres;
        Sphere s;
        s.center = { 1, 1, 0 };
        s.radius = 0.5;
        s.material = { { 1, 1, 1 }, 0, 0.1 };
        spheres.push_back(s);


        frameData.sphereNumber = spheres.size();

        Buffer sphereBuffer(sizeof(Sphere) * spheres.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        if (spheres.size() > 0)
        {
            copyData = sphereBuffer.Map();
            memcpy(copyData, spheres.data(), static_cast<size_t>(sizeof(Sphere) * spheres.size()));
            sphereBuffer.Unmap();
        }


        std::vector<Triangle> triangles;
        std::vector<Mesh> meshes;

        LoadModel("res/Meshes/plane.obj", triangles, meshes,  { {1, 1, 1}, 0, 0.8 }, { 0, 0, 0 }, { 2, 1, 2 });
        LoadModel("res/Meshes/cube.obj", triangles, meshes, { {0.9, 0.9, 0.9}, 0, 0.1 }, { -1, 1, 0 });

        frameData.meshNumber = meshes.size();
        Buffer triangleBuffer(sizeof(Triangle) * (triangles.size() + 1), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        Buffer meshBuffer(sizeof(Mesh) * (meshes.size() + 1), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (meshes.size() > 0)
        {
            copyData = triangleBuffer.Map();
            memcpy(copyData, triangles.data(), static_cast<size_t>(sizeof(Triangle) * triangles.size()));
            triangleBuffer.Unmap();

            copyData = meshBuffer.Map();
            memcpy(copyData, meshes.data(), static_cast<size_t>(sizeof(Mesh) * meshes.size()));
            meshBuffer.Unmap();
        }
        renderer->UpdateDescriptorSet(computeDescriptor, {
            { 0, DescriptorType::UniformBuffer, {computeFrameBuffer.GetHandle(), 0, sizeof(FrameData)}, {}},
            { 1, DescriptorType::StorageBuffer, {sphereBuffer.GetHandle(), 0, sizeof(Sphere) * spheres.size()}, {}},
            { 2, DescriptorType::StorageBuffer, {triangleBuffer.GetHandle(), 0, sizeof(Triangle) * triangles.size()}, {}},
            { 3, DescriptorType::StorageBuffer, {meshBuffer.GetHandle(), 0, sizeof(Mesh) * meshes.size()}, {}},
            { 4, DescriptorType::StorageImage, {}, {linearSampler.GetHandle(), raytracingImage.GetImageView(), VK_IMAGE_LAYOUT_GENERAL}},
            { 5, DescriptorType::StorageImage, {}, {linearSampler.GetHandle(), accumulationImage.GetImageView(), VK_IMAGE_LAYOUT_GENERAL}},
            });

        CameraFPS camera(window);

        /*
        VkQueryPool queryPool;
        VkQueryPoolCreateInfo queryPoolCreateInfo{};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.pNext = nullptr; // Optional
        queryPoolCreateInfo.flags = 0; // Reserved for future use, must be 0!

        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = 1 * 2; // REVIEW

        VkResult result = vkCreateQueryPool(Core::Get()->GetLogicalDevice(), &queryPoolCreateInfo, nullptr, &queryPool);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create time query pool!");
        }
        vkResetQueryPool(Core::Get()->GetLogicalDevice(), queryPool, 0, 2);
        */

        auto startTime = std::chrono::high_resolution_clock::now();

        auto frameStartTime = startTime;
        uint32_t frameIndex = 1;
        uint32_t timePerFrame = 10.0;
        uint32_t maxFrames = 16;
        while (!glfwWindowShouldClose(window))
        {
            //std::this_thread::sleep_for(std::chrono::milliseconds(4));
            glfwPollEvents();
            if (glfwGetKey(window, GLFW_KEY_ESCAPE))
                glfwSetWindowShouldClose(window, true);


            if (renderer->GetSwapchain()->OutOfDate())
            {
                int w, h;
                glfwGetWindowSize(window, &w, &h);
                std::cout << "Swapchain out of date! RECREATING -> width: " << w << " height: " << h << std::endl;
                renderer->GetSwapchain()->Recreate({(uint32_t)w, (uint32_t)h});
            }

            //std::cout << camera.position.x << " " << camera.position.y << " " << camera.position.z << std::endl;
            auto currentTime = std::chrono::high_resolution_clock::now();
            double deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            startTime = currentTime;

            /*
            if (std::chrono::duration<float, std::chrono::seconds::period>(currentTime - frameStartTime).count() > timePerFrame)
            {
                if (frameIndex <= maxFrames)
                {
                    Core::Get()->WaitIdle();

                    //Renderer::Get()->SaveScreenshot("Video-/" + std::to_string(frameIndex) + ".png");
                    frameIndex++;


                    float ratio = (float)frameIndex / (float)maxFrames;
                    float reverse = (1.0f - ratio);
                    glm::vec3 position = {4.0 * cos(6.283185 * reverse), 1.5, 4.0 * sin(6.283185 * reverse) };
                    glm::vec3 forward = glm::normalize(-position);

                    frameData.maxBouceLimit = frameIndex;
                    //frameData.cameraInverseProjection = glm::inverse(glm::perspectiveFov(70.0f, (float)windowExtent.width, (float)windowExtent.height, 0.1f, 1000.0f));
                    //frameData.cameraInverseView = glm::inverse(glm::lookAtLH({0, 0, 0}, position, {0.0, 1.0, 0.0}));
                    //frameData.cameraPos = glm::vec4(position.x, position.y, position.z, 0);
                    //frameData.cameraDirection = glm::vec4(forward.x, forward.y, forward.z, 0);

                    //frameData.skyColorZenith = glm::lerp(glm::vec4(0.2, 0.56, 0.95, 0.0), glm::vec4(0.0, 0.0, 0.0, 0.0), ratio);
                    //frameData.skyColorHorizon = LerpM(skyHorizonColors, ratio);
                    //frameData.skyColorZenith = LerpM(skyZenithColors, ratio);
                    //frameData.groundColor = LerpM(groundColors, ratio);
                    //frameData.sunIntensity = LerpM({1.0, 0.0, 1.0}, ratio);
                    frameData.frameIndex = 0;

                    //spheres[0].center = { 2 * cos(9 * 6.283185 * ratio), 2, 2 * sin(9 * 6.283185 * ratio) };
                    //spheres[0].radius = 0.5;
                    //spheres[0].material = { { 0.8, 0.6, 0.6 }, 1.0, 0.8 };
                    //spheres[1].center = { 0, 1, 1 };
                    //spheres[1].radius = 0.5;
                    //spheres[1].material = { { 0.6, 0.8, 0.6 }, 0, 0.8 };
                    //spheres[2].center = { 2, 1, 1 };
                    //spheres[2].radius = 0.5;
                    //spheres[2].material = { { 0.6, 0.6, 0.8 }, 1.0, 0.8 };

                    copyData = sphereBuffer.Map();
                    memcpy(copyData, spheres.data(), static_cast<size_t>(sizeof(Sphere) * spheres.size()));
                    sphereBuffer.Unmap();

                }
                frameStartTime = std::chrono::high_resolution_clock::now();
            }*/

            //std::cout << "Draw calls: " << renderer->renderStatistics.drawCalls << " "
            //    << "Vertices: " << renderer->renderStatistics.vertices << std::endl;


            if (glfwGetKey(window, GLFW_KEY_1))
            {
                Core::Get()->WaitIdle();
                SpirvHelper::Init();
                presentFrag->Reload("res/Shaders/present.frag");
                SpirvHelper::Finalize();
                presentPipeline.ReloadPipeline({
                        {
                            presentVert->GetShaderStage(),
                            presentFrag->GetShaderStage()
                        },
                        PrimitiveTopology::TriangleList,
                        renderer->GetSwapchain()->GetRenderPass(),
                        emptyVertexAttributes,
                        layout.GetHandle(),
                        1
                    });
                
            }
            if (glfwGetKey(window, GLFW_KEY_2))
            {
                Core::Get()->WaitIdle();
                Renderer::Get()->SaveScreenshot("Screenshot/test.png");
            }
            if (glfwGetKey(window, GLFW_KEY_Q))
            {
                frameData.frameIndex = 0;
            }

            VkCommandBuffer cmd = renderer->BeginFrame();
            //vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, 0);

            VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = static_cast<float>(renderer->GetSwapchain()->GetExtent().height);
            viewport.width = static_cast<float>(renderer->GetSwapchain()->GetExtent().width);
            viewport.height = -static_cast<float>(renderer->GetSwapchain()->GetExtent().height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissor = {};
            scissor.offset = { 0, 0 };
            scissor.extent = renderer->GetSwapchain()->GetExtent();
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            camera.Update(deltaTime);
            frameData.cameraInverseProjection = camera.inverseProjection;
            frameData.cameraInverseView = camera.inverseView;
            frameData.cameraPos = glm::vec4(camera.position.x, camera.position.y, camera.position.z, 0);
            frameData.cameraDirection = glm::vec4(camera.forward.x, camera.forward.y, camera.forward.z, 0);
            frameData.window.x = windowExtent.width;
            frameData.window.y = windowExtent.height;
            frameData.frameIndex++;
            if (camera.moved)
                frameData.frameIndex = 1;

            //frameData.skyColorHorizon = glm::vec4(camera.position.x, camera.position.x, camera.position.x, 0.0);
            //frameData.skyColorZenith = glm::vec4(0.2, 0.56, 0.95, 0.0);
            //frameData.sunLightDirection = glm::vec4(-0.4, -0.4, -0.4, 0.0);
            //frameData.sunFocus = camera.position.x;
            //frameData.sunIntensity = camera.position.x;
            copyData = computeFrameBuffer.Map();
            memcpy(copyData, &frameData, static_cast<size_t>(sizeof(FrameData)));
            computeFrameBuffer.Unmap();
            
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.GetHandle());
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.GetLayout(), 0, 1, &computeDescriptor, 0, nullptr);
            vkCmdDispatch(cmd, std::ceil(windowExtent.width / 64.0f), std::ceil(windowExtent.height / 16.0f), 1);
            

            VkMemoryBarrier memBarrier;
            memBarrier.pNext = nullptr;
            memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            memBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1, &memBarrier, 0, nullptr, 0, nullptr);


            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = renderer->GetSwapchain()->GetRenderPass();
            renderPassBeginInfo.framebuffer = renderer->GetSwapchain()->GetFramebuffer();

            renderPassBeginInfo.renderArea.offset = { 0, 0 };
            renderPassBeginInfo.renderArea.extent = renderer->GetSwapchain()->GetExtent();
            std::array<VkClearValue, 1> clearValues = {};
            clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
            renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassBeginInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, presentPipeline.GetLayout(), 0, 1, &descriptor, 0, nullptr);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, presentPipeline.GetHandle());
            vkCmdDraw(cmd, 6, 1, 0, 0);

            //ImGuiBeginFrame();
            //ImGui::Begin("Vulkan Texture", 0, ImGuiWindowFlags_NoScrollbar);
            //ImGui::Text("lol");
            //ImGui::End();
            //ImGuiEndFrame(cmd);

            vkCmdEndRenderPass(cmd);
            //vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 1);
      

            //swapchain->EndFrame();
            renderer->EndFrame();

            /*
            uint64_t buffer[2];

            Core::Get()->WaitIdle();

            VkResult result = vkGetQueryPoolResults(Core::Get()->GetLogicalDevice(), queryPool, 0, 2, sizeof(uint64_t) * 2, buffer, sizeof(uint64_t),
                VK_QUERY_RESULT_64_BIT);
            if (result == VK_NOT_READY)
            {
                std::cout << "RETURN" << std::endl;
                return 0;
            }
            else if (result == VK_SUCCESS)
            {
                float f = Core::Get()->GetPhysicalDeviceProperties().limits.timestampPeriod;
                //mTimeQueryResults[swapchainIndex] = buffer[1] - buffer[0];
                //std::cout << "Time: " << (buffer[1] - buffer[0]) * 0.0000001 * f << std::endl;
                std::cout << "Time: " << (buffer[1] - buffer[0]) * double(f / 1e6) << std::endl;
            }
            else
            {
                throw std::runtime_error("Failed to receive query results!");
            }

            // Queries must be reset after each individual use.
            vkResetQueryPool(Core::Get()->GetLogicalDevice(), queryPool, 0, 2);
            */
        }

        Core::Get()->WaitIdle();
        //vkDestroyQueryPool(Core::Get()->GetLogicalDevice(), queryPool, nullptr);
        ImGuiShutdown();
    }

	return 0;
}