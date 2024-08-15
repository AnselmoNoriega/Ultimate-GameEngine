#pragma once

#include "NotRed/Renderer/Framebuffer.h"

namespace NR
{
	class GLFrameBuffer : public FrameBuffer
	{
	public:
		GLFrameBuffer(uint32_t width, uint32_t height, FrameBufferFormat format);
		~GLFrameBuffer() override;

		void Resize(uint32_t width, uint32_t height) override;

		void Bind() const override;
		void Unbind() const override;

		void BindTexture(uint32_t slot = 0) const override;

		RendererID GetRendererID() const { return mID; }
		RendererID GetColorAttachmentRendererID() const { return mColorAttachment; }
		RendererID GetDepthAttachmentRendererID() const { return mDepthAttachment; }

		uint32_t GetWidth() const { return mWidth; }
		uint32_t GetHeight() const { return mHeight; }
		FrameBufferFormat GetFormat() const { return mFormat; }

	private:
		RendererID mID = 0;
		uint32_t mWidth, mHeight;
		FrameBufferFormat mFormat;

		RendererID mColorAttachment, mDepthAttachment;
	};
}