#include "nrpch.h"
#include "GLFrameBuffer.h"

#include "NotRed/Renderer/Renderer.h"
#include <glad/glad.h>

namespace NR
{
	GLFrameBuffer::GLFrameBuffer(const FrameBufferSpecification& spec)
		: mSpecification(spec)
	{
		Resize(spec.Width, spec.Height);
	}

	GLFrameBuffer::~GLFrameBuffer()
	{
		NR_RENDER_S({
			glDeleteFramebuffers(1, &self->mID);
			});
	}

	void GLFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		if (mSpecification.Width == width && mSpecification.Height == height)
		{
			return;
		}

		mSpecification.Width = width;
		mSpecification.Height = height;

		NR_RENDER_S({
			if (self->mID)
			{
				glDeleteFramebuffers(1, &self->mID);
				glDeleteTextures(1, &self->mColorAttachment);
				glDeleteTextures(1, &self->mDepthAttachment);
			}

			glGenFramebuffers(1, &self->mID);
			glBindFramebuffer(GL_FRAMEBUFFER, self->mID);

			glGenTextures(1, &self->mColorAttachment);
			glBindTexture(GL_TEXTURE_2D, self->mColorAttachment);

			if (self->mSpecification.Format == FrameBufferFormat::RGBA16F)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, self->mSpecification.Width, self->mSpecification.Height, 0, GL_RGBA, GL_FLOAT, nullptr);
			}
			else if (self->mSpecification.Format == FrameBufferFormat::RGBA8)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->mSpecification.Width, self->mSpecification.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->mColorAttachment, 0);

			glGenTextures(1, &self->mDepthAttachment);
			glBindTexture(GL_TEXTURE_2D, self->mDepthAttachment);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, self->mSpecification.Width, self->mSpecification.Height, 0,
				GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
			);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, self->mDepthAttachment, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				NR_CORE_ERROR("Framebuffer is incomplete!");
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			});
	}

	void GLFrameBuffer::Bind() const
	{
		NR_RENDER_S({
			glBindFramebuffer(GL_FRAMEBUFFER, self->mID);
			glViewport(0, 0, self->mSpecification.Width, self->mSpecification.Height);
			});
	}

	void GLFrameBuffer::Unbind() const
	{
		NR_RENDER_S({
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			});
	}

	void GLFrameBuffer::BindTexture(uint32_t slot) const
	{
		NR_RENDER_S1(slot, {
			glActiveTexture(GL_TEXTURE0 + slot);
			glBindTexture(GL_TEXTURE_2D, self->mColorAttachment);
			});
	}
}