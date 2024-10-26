#include "nrpch.h"
#include "VKImage.h"

#include "VKContext.h"

namespace NR
{
	namespace Utils 
	{
		static void InsertImageMemoryBarrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange)
		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.srcAccessMask = srcAccessMask;
			imageMemoryBarrier.dstAccessMask = dstAccessMask;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}
	}

	VKImage2D::VKImage2D(ImageSpecification specification)
		: mSpecification(specification)
	{
	}

	VKImage2D::~VKImage2D()
	{
		if (mInfo.Image)
		{
			const VKImageInfo& info = mInfo;
			Renderer::Submit([info, layerViews = mPerLayerImageViews]()
				{
					const auto vulkanDevice = VKContext::GetCurrentDevice()->GetVulkanDevice();
					vkDestroyImageView(vulkanDevice, info.ImageView, nullptr);
					vkDestroySampler(vulkanDevice, info.Sampler, nullptr);
					for (auto& view : layerViews)
					{
						if (view)
						{
							vkDestroyImageView(vulkanDevice, view, nullptr);
						}
					}
					VKAllocator allocator("VulkanImage2D");
					allocator.DestroyImage(info.Image, info.MemoryAlloc);
					NR_CORE_WARN("VulkanImage2D::Release ImageView = {0}", (const void*)info.ImageView);
				});

			mPerLayerImageViews.clear();
		}
	}

	void VKImage2D::Invalidate()
	{
		Ref<VKImage2D> instance = this;
		Renderer::Submit([instance]() mutable
			{
				instance->RT_Invalidate();
			});
	}

	void VKImage2D::Release()
	{
		VKImageInfo info = mInfo;
		Renderer::SubmitResourceFree([info, layerViews = mPerLayerImageViews]() mutable
			{
				const auto vulkanDevice = VKContext::GetCurrentDevice()->GetVulkanDevice();
				NR_CORE_WARN("VulkanImage2D::Release ImageView = {0}", (const void*)info.ImageView);
				vkDestroyImageView(vulkanDevice, info.ImageView, nullptr);
				vkDestroySampler(vulkanDevice, info.Sampler, nullptr);
				for (auto& view : layerViews)
				{
					if (view)
					{
						vkDestroyImageView(vulkanDevice, view, nullptr);
					}
				}
				VKAllocator allocator("VulkanImage2D");
				allocator.DestroyImage(info.Image, info.MemoryAlloc);
			});

		mInfo.Image = nullptr;
		mInfo.ImageView = nullptr;
		mInfo.Sampler = nullptr;
		mPerLayerImageViews.clear();
	}

	void VKImage2D::RT_Invalidate()
	{
		VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
		VKAllocator allocator("Image2D");
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;

		if (mSpecification.Usage == ImageUsage::Attachment)
		{
			if (Utils::IsDepthFormat(mSpecification.Format))
			{
				usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			}
			else
			{
				usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			}
		}
		else if (mSpecification.Usage == ImageUsage::Texture)
		{
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		else if (mSpecification.Usage == ImageUsage::Storage)
		{
			usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		VkImageAspectFlags aspectMask = Utils::IsDepthFormat(mSpecification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		if (mSpecification.Format == ImageFormat::DEPTH24STENCIL8)
		{
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		VkFormat vulkanFormat = Utils::VulkanImageFormat(mSpecification.Format);
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = vulkanFormat;
		imageCreateInfo.extent.width = mSpecification.Width;
		imageCreateInfo.extent.height = mSpecification.Height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = mSpecification.Mips;
		imageCreateInfo.arrayLayers = mSpecification.Layers;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = usage;
		mInfo.MemoryAlloc = allocator.AllocateImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, mInfo.Image);

		// Create a default image view
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.viewType = mSpecification.Layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = vulkanFormat;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.subresourceRange = {};
		imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = mSpecification.Mips;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = mSpecification.Layers;
		imageViewCreateInfo.image = mInfo.Image;
		VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &mInfo.ImageView));

		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 1.0f;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &mInfo.Sampler));

		if (mSpecification.Usage == ImageUsage::Storage)
		{
			// Transition image to GENERAL layout
			VkCommandBuffer commandBuffer = VKContext::GetCurrentDevice()->GetCommandBuffer(true);
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = mSpecification.Mips;
			subresourceRange.layerCount = mSpecification.Layers;

			Utils::InsertImageMemoryBarrier(
				commandBuffer, mInfo.Image,
				0, 0,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				subresourceRange);

			VKContext::GetCurrentDevice()->FlushCommandBuffer(commandBuffer);
		}

		UpdateDescriptor();
	}

	void VKImage2D::CreatePerLayerImageViews()
	{
		Ref<VKImage2D> instance = this;
		Renderer::Submit([instance]() mutable
			{
				instance->RT_CreatePerLayerImageViews();
			});
	}

	void VKImage2D::RT_CreatePerLayerImageViews()
	{
		NR_CORE_ASSERT(mSpecification.Layers > 1);

		VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

		VkImageAspectFlags aspectMask = Utils::IsDepthFormat(mSpecification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		if (mSpecification.Format == ImageFormat::DEPTH24STENCIL8)
		{
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		const VkFormat vulkanFormat = Utils::VulkanImageFormat(mSpecification.Format);
		mPerLayerImageViews.resize(mSpecification.Layers);
		for (uint32_t layer = 0; layer < mSpecification.Layers; layer++)
		{
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = vulkanFormat;
			imageViewCreateInfo.flags = 0;
			imageViewCreateInfo.subresourceRange = {};
			imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = mSpecification.Mips;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = layer;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.image = mInfo.Image;
			VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &mPerLayerImageViews[layer]));
		}
	}

	void VKImage2D::RT_CreatePerSpecificLayerImageViews(const std::vector<uint32_t>& layerIndices)
	{
		NR_CORE_ASSERT(mSpecification.Layers > 1);
		VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
		VkImageAspectFlags aspectMask = Utils::IsDepthFormat(mSpecification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		
		if (mSpecification.Format == ImageFormat::DEPTH24STENCIL8)
		{
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		const VkFormat vulkanFormat = Utils::VulkanImageFormat(mSpecification.Format);
		
		if (mPerLayerImageViews.empty())
		{
			mPerLayerImageViews.resize(mSpecification.Layers);
		}

		for (uint32_t layer : layerIndices)
		{
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = vulkanFormat;
			imageViewCreateInfo.flags = 0;
			imageViewCreateInfo.subresourceRange = {};
			imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = mSpecification.Mips;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = layer;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.image = mInfo.Image;
			VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &mPerLayerImageViews[layer]));
		}
	}

	void VKImage2D::UpdateDescriptor()
	{
		if (mSpecification.Format == ImageFormat::DEPTH24STENCIL8 || mSpecification.Format == ImageFormat::DEPTH32F)
		{
			mDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		}
		else
		{
			mDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		if (mSpecification.Usage == ImageUsage::Storage)
		{
			mDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		}

		mDescriptorImageInfo.imageView = mInfo.ImageView;
		mDescriptorImageInfo.sampler = mInfo.Sampler;
	}
}