#pragma once

#include "NotRed/Renderer/Framebuffer.h"

namespace NR
{
	class GLFrameBuffer : public FrameBuffer
	{
	public:
		GLFrameBuffer(const FrameBufferSpecification& spec);
		~GLFrameBuffer() override;

		void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;

		void Bind() const override;
		void Unbind() const override;

		void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override;

		uint32_t GetWidth() const override { return mSpecification.Width; }
		uint32_t GetHeight() const override { return mSpecification.Height; }

		RendererID GetRendererID() const { return mID; }
		RendererID GetColorAttachmentRendererID(int index = 0) const { return mColorAttachments[index]; }
		RendererID GetDepthAttachmentRendererID() const { return mDepthAttachment; }

		const FrameBufferSpecification& GetSpecification() const override { return mSpecification; }

	private:
		RendererID mID = 0;
		FrameBufferSpecification mSpecification;

		std::vector<RendererID> mColorAttachments;
		RendererID mDepthAttachment;

		std::vector<FrameBufferTextureFormat> mColorAttachmentFormats;
		FrameBufferTextureFormat mDepthAttachmentFormat = FrameBufferTextureFormat::None;

		uint32_t mWidth = 0, mHeight = 0;
	};
}