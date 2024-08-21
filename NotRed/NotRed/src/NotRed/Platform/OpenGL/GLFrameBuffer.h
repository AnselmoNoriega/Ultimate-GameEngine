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

		void BindTexture(uint32_t slot = 0) const override;

		RendererID GetRendererID() const { return mID; }
		RendererID GetColorAttachmentRendererID() const { return mColorAttachment; }
		RendererID GetDepthAttachmentRendererID() const { return mDepthAttachment; }

		const FrameBufferSpecification& GetSpecification() const override { return mSpecification; }

	private:
		RendererID mID = 0;
		FrameBufferSpecification mSpecification;

		RendererID mColorAttachment = 0, mDepthAttachment = 0;
	};
}