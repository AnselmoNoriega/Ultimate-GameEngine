#pragma once

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Renderer/Texture.h"

namespace NR
{
	class GLTexture2D : public Texture2D
	{
	public:
		GLTexture2D(TextureFormat format, uint32_t width, uint32_t height);
		GLTexture2D(const std::string& path, bool standardRGB);
		~GLTexture2D() override;

		void Bind(uint32_t slot = 0) const override;

		TextureFormat GetFormat() const override { return mFormat; }
		uint32_t GetWidth() const override { return mWidth; }
		uint32_t GetHeight() const override { return mHeight; }

		const std::string& GetPath() const override { return mFilePath; }

		RendererID GetRendererID() const override { return mID; }

	private:
		RendererID mID;

		TextureFormat mFormat;
		uint32_t mWidth, mHeight;
		unsigned char* mImageData;

		std::string mFilePath;
	};

	class GLTextureCube : public TextureCube
	{
	public:
		GLTextureCube(const std::string& path);
		virtual ~GLTextureCube() override;

		void Bind(uint32_t slot = 0) const override;

		TextureFormat GetFormat() const override { return mFormat; }
		uint32_t GetWidth() const override { return mWidth; }
		uint32_t GetHeight() const override { return mHeight; }

		const std::string& GetPath() const override { return mFilePath; }

		RendererID GetRendererID() const override { return mID; }

	private:
		RendererID mID;

		TextureFormat mFormat;
		uint32_t mWidth, mHeight;
		unsigned char* mImageData;

		std::string mFilePath;
	};
}