#include "nrpch.h"
#include "GLTexture.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Renderer/Renderer.h"

#include <Glad/glad.h>

namespace NR
{
	static GLenum ToOpenGLTextureFormat(TextureFormat format)
	{
		switch (format)
		{
		case NR::TextureFormat::RGB:     return GL_RGB;
		case NR::TextureFormat::RGBA:    return GL_RGBA;
		}
		return 0;
	}

	GLTexture2D::GLTexture2D(TextureFormat format, uint32_t width, uint32_t height)
		: mFormat(format), mWidth(width), mHeight(height)
	{
		auto self = this;
		NR_RENDER_1(self, {
			glGenTextures(1, &self->mID);
			glBindTexture(GL_TEXTURE_2D, self->mID);
			glTexImage2D(GL_TEXTURE_2D, 0, ToOpenGLTextureFormat(self->mFormat), self->mWidth, self->mHeight, 0, ToOpenGLTextureFormat(self->mFormat), GL_UNSIGNED_BYTE, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);
			});
	}

	GLTexture2D::~GLTexture2D()
	{
		auto self = this;
		NR_RENDER_1(self, {
			glDeleteTextures(1, &self->mID);
			});
	}
}