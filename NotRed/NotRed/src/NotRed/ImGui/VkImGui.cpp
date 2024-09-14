#include "nrpch.h"
#include "ImGui.h"

#include "NotRed/Renderer/RendererAPI.h"

#include "NotRed/Platform/Vulkan/VkTexture.h"
#include "NotRed/Platform/Vulkan/VkImage.h"

#include "NotRed/Platform/GL/GLTexture.h"
#include "NotRed/Platform/GL/GLImage.h"

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
			Ref<VkImage2D> VkImage = image.As<VkImage2D>();
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
			Ref<VkTexture2D> VkTexture = texture.As<VkTexture2D>();
			const VkDescriptorImageInfo& imageInfo = VkTexture->GetVkDescriptorInfo();
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
			Ref<VkImage2D> VkImage = image.As<VkImage2D>();
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
			Ref<VkTexture2D> VkTexture = texture.As<VkTexture2D>();
			const VkDescriptorImageInfo& imageInfo = VkTexture->GetVkDescriptorInfo();
			auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
			return ImGui::ImageButton((ImTextureID)textureID, size, uv0, uv1, frame_padding, bg_col, tint_col);
		}
	}

}