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
		~VKFrameBuffer() override;

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

		Ref<Image2D> GetImage(uint32_t attachmentIndex = 0) const override { NR_CORE_ASSERT(attachmentIndex < mAttachments.size()); return mAttachments[attachmentIndex]; }
		Ref<Image2D> GetDepthImage() const override { return mDepthAttachment; }

		size_t GetColorAttachmentCount() const { return mAttachments.size(); }

		VkRenderPass GetRenderPass() const { return mRenderPass; }
		VkFramebuffer GetVulkanFrameBuffer() const { return mFrameBuffer; }
		const std::vector<VkClearValue>& GetVulkanClearValues() const { return mClearValues; }

		const FrameBufferSpecification& GetSpecification() const override { return mSpecification; }
		
	private:
		FrameBufferSpecification mSpecification;
		RendererID mID = 0;
		uint32_t mWidth = 0, mHeight = 0;

		std::vector<Ref<Image2D>> mAttachments;
		Ref<Image2D> mDepthAttachment;

		std::vector<VkClearValue> mClearValues;

		VkSampler mColorAttachmentSampler;
		VkRenderPass mRenderPass = nullptr;
		VkFramebuffer mFrameBuffer = nullptr;

		std::vector<std::function<void(Ref<FrameBuffer>)>> mResizeCallbacks;
	};
}