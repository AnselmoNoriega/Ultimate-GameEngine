#include <nrpch.h>
#include "ImGuiUtil.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Platform/OpenGL/GLImage.h"
#include "NotRed/Platform/Vulkan/VKTexture.h"

#include "imgui/examples/imgui_impl_vulkan_with_textures.h"

namespace NR::UI
{
    ImTextureID GetTextureID(Ref<Texture2D> texture)
    {
        if (RendererAPI::Current() == RendererAPIType::OpenGL)
        {
            Ref<GLImage2D> image = texture->GetImage().As<GLImage2D>();
            return (ImTextureID)(size_t)image->GetRendererID();
        }
        else
        {
            Ref<VKTexture2D> vulkanTexture = texture.As<VKTexture2D>();
            const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetVulkanDescriptorInfo();
            if (!imageInfo.imageView)
            {
                return nullptr;
            }

            return ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
        }
    }
}