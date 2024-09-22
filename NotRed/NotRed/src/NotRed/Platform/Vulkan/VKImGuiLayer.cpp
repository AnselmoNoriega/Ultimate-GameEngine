#include "nrpch.h"
#include "VKImGuiLayer.h"

#include <GLFW/glfw3.h>

#include "imgui.h"

#define IMGUI_IMPL_API
#include "backends/imgui_impl_glfw.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

#include "NotRed/ImGui/ImGuizmo.h"

#include "NotRed/Platform/Vulkan/VKContext.h"

#include "NotRed/Core/Application.h"
#include "NotRed/Renderer/Renderer.h"

namespace NR
{
    static VkCommandBuffer sImGuiCommandBuffer;

    VKImGuiLayer::VKImGuiLayer()
    {
    }

    VKImGuiLayer::VKImGuiLayer(const std::string& name)
    {

    }

    VKImGuiLayer::~VKImGuiLayer()
    {
    }

    void VKImGuiLayer::Attach()
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        io.Fonts->AddFontFromFileTTF("Assets/Fonts/Roboto/Roboto-Bold.ttf", 18.0f);
        io.Fonts->AddFontFromFileTTF("Assets/Fonts/Roboto/Roboto-Regular.ttf", 24.0f);
        io.FontDefault = io.Fonts->AddFontFromFileTTF("Assets/Fonts/Roboto/Roboto-Regular.ttf", 18.0f);

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        SetDarkThemeColors();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

        VKImGuiLayer* instance = this;
        Renderer::Submit([instance]()
            {
                Application& app = Application::Get();
                GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

                auto vulkanContext = VKContext::Get();
                auto device = VKContext::GetCurrentDevice()->GetVulkanDevice();

                VkDescriptorPool descriptorPool;

                VkDescriptorPoolSize poolSizes[] =
                {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                };
                VkDescriptorPoolCreateInfo poolInfo = {};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
                poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
                poolInfo.pPoolSizes = poolSizes;
                VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));

                // Setup Platform/Renderer bindings
                ImGui_ImplGlfw_InitForVulkan(window, true);
                ImGui_ImplVulkan_InitInfo initInfo = {};
                initInfo.Instance = VKContext::GetInstance();
                initInfo.PhysicalDevice = VKContext::GetCurrentDevice()->GetPhysicalDevice()->GetVulkanPhysicalDevice();
                initInfo.Device = device;
                initInfo.QueueFamily = VKContext::GetCurrentDevice()->GetPhysicalDevice()->GetQueueFamilyIndices().Graphics;
                initInfo.Queue = VKContext::GetCurrentDevice()->GetQueue();
                initInfo.PipelineCache = nullptr;
                initInfo.DescriptorPool = descriptorPool;
                initInfo.Allocator = nullptr;
                initInfo.MinImageCount = 2;
                initInfo.ImageCount = vulkanContext->GetSwapChain().GetImageCount();
                initInfo.CheckVkResultFn = Utils::VulkanCheckResult;
                ImGui_ImplVulkan_Init(&initInfo, vulkanContext->GetSwapChain().GetRenderPass());

                // Load Fonts
                // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
                // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
                // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
                // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
                // - Read 'docs/FONTS.md' for more instructions and details.
                // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
                //io.Fonts->AddFontDefault();
                //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
                //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
                //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
                //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
                //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
                //IM_ASSERT(font != NULL);

                // Upload Fonts
                {
                    VkCommandBuffer commandBuffer = vulkanContext->GetCurrentDevice()->GetCommandBuffer(true);
                    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
                    vulkanContext->GetCurrentDevice()->FlushCommandBuffer(commandBuffer);

                    VK_CHECK_RESULT(vkDeviceWaitIdle(device));
                    ImGui_ImplVulkan_DestroyFontUploadObjects();
                }

                sImGuiCommandBuffer = VKContext::GetCurrentDevice()->CreateSecondaryCommandBuffer();
            });
    }

    void VKImGuiLayer::Detach()
    {
        Renderer::Submit([]()
            {
                auto device = VKContext::GetCurrentDevice()->GetVulkanDevice();

                VK_CHECK_RESULT(vkDeviceWaitIdle(device));
                ImGui_ImplVulkan_Shutdown();
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
            });
    }

    void VKImGuiLayer::Begin()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void VKImGuiLayer::End()
    {
        ImGui::Render();

        Ref<VKContext> context = VKContext::Get();
        VKSwapChain& swapChain = context->GetSwapChain();
        VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();

        VkClearValue clearValues[2];
        clearValues[0].color = { {0.1f, 0.1f,0.1f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        uint32_t width = swapChain.GetWidth();
        uint32_t height = swapChain.GetHeight();

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = width;
        renderPassBeginInfo.renderArea.extent.height = height;
        renderPassBeginInfo.clearValueCount = 2; // Color + depth
        renderPassBeginInfo.pClearValues = clearValues;
        renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

        vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.renderPass = swapChain.GetRenderPass();
        inheritanceInfo.framebuffer = swapChain.GetCurrentFramebuffer();

        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

        VK_CHECK_RESULT(vkBeginCommandBuffer(sImGuiCommandBuffer, &cmdBufInfo));

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = (float)height;
        viewport.height = -(float)height;
        viewport.width = (float)width;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(sImGuiCommandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.extent.width = width;
        scissor.extent.height = height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(sImGuiCommandBuffer, 0, 1, &scissor);

        ImDrawData* main_draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(main_draw_data, sImGuiCommandBuffer);

        VK_CHECK_RESULT(vkEndCommandBuffer(sImGuiCommandBuffer));

        std::vector<VkCommandBuffer> commandBuffers;
        commandBuffers.push_back(sImGuiCommandBuffer);

        vkCmdExecuteCommands(drawCommandBuffer, commandBuffers.size(), commandBuffers.data());

        vkCmdEndRenderPass(drawCommandBuffer);

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void VKImGuiLayer::ImGuiRender()
    {
    }

}