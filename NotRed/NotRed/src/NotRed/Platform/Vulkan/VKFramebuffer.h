#pragma once

#include "Vulkan.h"
#include "VKImage.h"

#include "NotRed/Renderer/FrameBuffer.h"

namespace NR
{
	class VKFrameBuffer : public FrameBuffer
	{
	public:
		VKFrameBuffer(const FrameBufferSpecification& spec);
		~VKFrameBuffer() override = default;

		void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;
		void AddResizeCallback(const std::function<void(Ref<FrameBuffer>)>& func) override;

		void Bind() const override {}
		void Unbind() const override {}

		void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override {}

		uint32_t GetWidth() const override { return mWidth; }
		uint32_t GetHeight() const override { return mHeight; }
		RendererID GetRendererID() const override { return mID; }
		RendererID GetColorAttachmentRendererID() const { return 0; }
		RendererID GetDepthAttachmentRendererID() const { return 0; }

		Ref<Image2D> GetImage(uint32_t attachmentIndex = 0) const override { NR_CORE_ASSERT(attachmentIndex < mAttachmentImages.size()); return mAttachmentImages[attachmentIndex]; }
		Ref<Image2D> GetDepthImage() const override { return mDepthAttachmentImage; }

		size_t GetColorAttachmentCount() const { return mSpecification.SwapChainTarget ? 1 : mAttachmentImages.size(); }
		bool HasDepthAttachment() const { return (bool)mDepthAttachmentImage; }

		VkRenderPass GetRenderPass() const { return mRenderPass; }
		VkFramebuffer GetVulkanFrameBuffer() const { return mFrameBuffer; }
		const std::vector<VkClearValue>& GetVulkanClearValues() const { return mClearValues; }

		const FrameBufferSpecification& GetSpecification() const override { return mSpecification; }
		
		void Invalidate();
		void RT_Invalidate();

	private:
		FrameBufferSpecification mSpecification;
		RendererID mID = 0;
		uint32_t mWidth = 0, mHeight = 0;

		std::vector<Ref<Image2D>> mAttachmentImages;
		Ref<Image2D> mDepthAttachmentImage;

		std::vector<VkClearValue> mClearValues;

		VkRenderPass mRenderPass = nullptr;
		VkFramebuffer mFrameBuffer = nullptr;

		std::vector<std::function<void(Ref<FrameBuffer>)>> mResizeCallbacks;
	};
}