#include "nrpch.h"
#include "ImGui.h"

#include "NotRed/Renderer/RendererAPI.h"

#include "NotRed/Platform/Vulkan/VkTexture.h"
#include "NotRed/Platform/Vulkan/VkImage.h"

#include "NotRed/Platform/OpenGL/GLTexture.h"
#include "NotRed/Platform/OpenGL/GLImage.h"

#include "imgui/examples/imgui_impl_vulkan_with_textures.h"

namespace NR::UI 
{
	void Image(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<GLImage2D> glImage = image.As<GLImage2D>();
			ImGui::Image((ImTextureID)glImage->GetRendererID(), size, uv0, uv1, tint_col, border_col);
		}
		else
		{
			Ref<VKImage2D> VkImage = image.As<VKImage2D>();
			const auto& imageInfo = VkImage->GetImageInfo();
			if (!imageInfo.ImageView)
			{
				return;
			}

			auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, VkImage->GetDescriptor().imageLayout);
			ImGui::Image((ImTextureID)textureID, size, uv0, uv1, tint_col, border_col);
		}
	}

	void Image(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<GLImage2D> image = texture->GetImage().As<GLImage2D>();
			ImGui::Image((ImTextureID)image->GetRendererID(), size, uv0, uv1, tint_col, border_col);
		}
		else
		{
			Ref<VKTexture2D> VkTexture = texture.As<VKTexture2D>();
			const VkDescriptorImageInfo& imageInfo = VkTexture->GetVulkanDescriptorInfo();
			auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
			ImGui::Image((ImTextureID)textureID, size, uv0, uv1, tint_col, border_col);
		}
	}

	bool ImageButton(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<GLImage2D> glImage = image.As<GLImage2D>();
			return ImGui::ImageButton((ImTextureID)glImage->GetRendererID(), size, uv0, uv1, frame_padding, bg_col, tint_col);
		}
		else
		{
			Ref<VKImage2D> VkImage = image.As<VKImage2D>();
			const auto& imageInfo = VkImage->GetImageInfo();
			if (!imageInfo.ImageView)
				return false;
			auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, VkImage->GetDescriptor().imageLayout);
			return ImGui::ImageButton((ImTextureID)textureID, size, uv0, uv1, frame_padding, bg_col, tint_col);
		}
	}

	bool ImageButton(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<GLImage2D> image = texture->GetImage().As<GLImage2D>();
			return ImGui::ImageButton((ImTextureID)image->GetRendererID(), size, uv0, uv1, frame_padding, bg_col, tint_col);
		}
		else
		{
			Ref<VKTexture2D> VkTexture = texture.As<VKTexture2D>();
			const VkDescriptorImageInfo& imageInfo = VkTexture->GetVulkanDescriptorInfo();
			auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
			return ImGui::ImageButton((ImTextureID)textureID, size, uv0, uv1, frame_padding, bg_col, tint_col);
		}
	}

}