#pragma once

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Renderer/Texture.h"

namespace NR
{
	class NOT_RED_API GLTexture2D : public Texture2D
	{
	public:
		GLTexture2D(TextureFormat format, uint32_t width, uint32_t height);
		~GLTexture2D() override;

		TextureFormat GetFormat() const override { return mFormat; }
		uint32_t GetWidth() const override { return mWidth; }
		uint32_t GetHeight() const override { return mHeight; }

	private:
		RendererID mID;
		TextureFormat mFormat;
		unsigned int mWidth, mHeight;
	};
}