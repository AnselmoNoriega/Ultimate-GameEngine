#include "nrpch.h"
#include "VKImage.h"

#include "VKContext.h"

namespace NR
{
	VKImage2D::VKImage2D(ImageFormat format, uint32_t width, uint32_t height)
		: mFormat(format), mWidth(width), mHeight(height)
	{
	}

	VKImage2D::~VKImage2D()
	{
	}

	void VKImage2D::Invalidate()
	{

	}

	void VKImage2D::Release()
	{
		auto vulkanDevice = VKContext::GetCurrentDevice()->GetVulkanDevice();
		vkDestroyImageView(vulkanDevice, mInfo.ImageView, nullptr);
		vkDestroyImage(vulkanDevice, mInfo.Image, nullptr);
		vkDestroySampler(vulkanDevice, mInfo.Sampler, nullptr);
		vkFreeMemory(vulkanDevice, mInfo.Memory, nullptr);

		NR_CORE_WARN("VKImage2D::Release ImageView = {0}", (const void*)mInfo.ImageView);
	}

	void VKImage2D::UpdateDescriptor()
	{
		if (mFormat == ImageFormat::DEPTH24STENCIL8 || mFormat == ImageFormat::DEPTH32F)
		{
			mDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		}
		else
		{
			mDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		mDescriptorImageInfo.imageView = mInfo.ImageView;
		mDescriptorImageInfo.sampler = mInfo.Sampler;

		NR_CORE_WARN("VKImage2D::UpdateDescriptor to ImageView = {0}", (const void*)mInfo.ImageView);
	}
}