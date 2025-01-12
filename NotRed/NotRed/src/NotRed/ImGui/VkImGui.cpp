#include "nrpch.h"
#include "ImGui.h"

#include "NotRed/Renderer/RendererAPI.h"

#include "NotRed/Platform/Vulkan/VkTexture.h"
#include "NotRed/Platform/Vulkan/VkImage.h"

#include "NotRed/Platform/OpenGL/GLTexture.h"
#include "NotRed/Platform/OpenGL/GLImage.h"

#include "imgui/examples/imgui_impl_vulkan_with_textures.h"

namespace ImGui 
{
	extern bool ImageButtonEx(ImGuiID id, ImTextureID texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec2& padding, const ImVec4& bg_col, const ImVec4& tint_col);
}

namespace NR::UI 
{
	void Image(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<GLImage2D> glImage = image.As<GLImage2D>();
			ImGui::Image((ImTextureID)(size_t)glImage->GetRendererID(), size, uv0, uv1, tint_col, border_col);
		}
		else
		{
			Ref<VKImage2D> vkImage = image.As<VKImage2D>();
			const auto& imageInfo = vkImage->GetImageInfo();
			if (!imageInfo.ImageView)
			{
				return;
			}

			const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vkImage->GetDescriptor().imageLayout);
			ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
		}
	}

	void Image(const Ref<Image2D>& image, uint32_t imageLayer, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<GLImage2D> glImage = image.As<GLImage2D>();
			ImGui::Image((ImTextureID)(size_t)glImage->GetRendererID(), size, uv0, uv1, tint_col, border_col);
		}
		else
		{
			Ref<VKImage2D> vulkanImage = image.As<VKImage2D>();
			auto imageInfo = vulkanImage->GetImageInfo();

			imageInfo.ImageView = vulkanImage->GetLayerImageView(imageLayer);
			if (!imageInfo.ImageView)
			{
				return;
			}
			const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptor().imageLayout);
			ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
		}
	}

	void Image(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<GLImage2D> image = texture->GetImage().As<GLImage2D>();
			ImGui::Image((ImTextureID)(size_t)image->GetRendererID(), size, uv0, uv1, tint_col, border_col);
		}
		else
		{
			Ref<VKTexture2D> VkTexture = texture.As<VKTexture2D>();
			const VkDescriptorImageInfo& imageInfo = VkTexture->GetVulkanDescriptorInfo();
			if (!imageInfo.imageView)
			{
				return;
			}

			const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
			ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
		}
	}

	void ImageMip(const Ref<Image2D>& image, uint32_t mip, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		Ref<VKImage2D> vulkanImage = image.As<VKImage2D>();
		auto imageInfo = vulkanImage->GetImageInfo();
		imageInfo.ImageView = vulkanImage->GetMipImageView(mip);
		
		if (!imageInfo.ImageView)
		{
			return;
		}

		const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptor().imageLayout);
		ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
	}

	bool ImageButton(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		return ImageButton(nullptr, image, size, uv0, uv1, frame_padding, bg_col, tint_col);
	}

	bool ImageButton(const char* stringID, const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<GLImage2D> glImage = image.As<GLImage2D>();
			return ImGui::ImageButton((ImTextureID)(size_t)glImage->GetRendererID(), size, uv0, uv1, frame_padding, bg_col, tint_col);
		}
		else
		{
			Ref<VKImage2D> VkImage = image.As<VKImage2D>();
			const auto& imageInfo = VkImage->GetImageInfo();
			if (!imageInfo.ImageView)
			{
				return false;
			}
			const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, VkImage->GetDescriptor().imageLayout);
			ImGuiID id = (ImGuiID)((((uint64_t)imageInfo.ImageView) >> 32) ^ (uint32_t)imageInfo.ImageView);
			if (stringID)
			{
				const ImGuiID strID = ImGui::GetID(stringID);
				id = id ^ strID;
			}
			return ImGui::ImageButtonEx(id, textureID, size, uv0, uv1, ImVec2{ (float)frame_padding, (float)frame_padding }, bg_col, tint_col);
		}
	}

	bool ImageButton(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		return ImageButton(nullptr, texture, size, uv0, uv1, frame_padding, bg_col, tint_col);
	}

	bool ImageButton(const char* stringID, const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		NR_CORE_VERIFY(texture);
		if (!texture)
		{
			return false;
		}

		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<GLImage2D> image = texture->GetImage().As<GLImage2D>();
			return ImGui::ImageButton((ImTextureID)(size_t)image->GetRendererID(), size, uv0, uv1, frame_padding, bg_col, tint_col);
		}
		else
		{
			Ref<VKTexture2D> vkTexture = texture.As<VKTexture2D>();

			NR_CORE_VERIFY(vkTexture->GetImage());
			if (!vkTexture->GetImage())
			{
				return false;
			}

			const VkDescriptorImageInfo& imageInfo = vkTexture->GetVulkanDescriptorInfo();
			const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);

			ImGuiID id = (ImGuiID)((((uint64_t)imageInfo.imageView) >> 32) ^ (uint32_t)imageInfo.imageView);
			if (stringID)
			{
				const ImGuiID strID = ImGui::GetID(stringID);
				id = id ^ strID;
			}

			return ImGui::ImageButtonEx(id, textureID, size, uv0, uv1, ImVec2{ (float)frame_padding, (float)frame_padding }, bg_col, tint_col);
		}
	}
}