#pragma once

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Renderer/Texture.h"

namespace NR
{
	class GLTexture2D : public Texture2D
	{
	public:
		GLTexture2D(TextureFormat format, uint32_t width, uint32_t height, TextureWrap wrap);
		GLTexture2D(const std::string& path, bool standardRGB);
		~GLTexture2D() override;

		void Bind(uint32_t slot = 0) const override;

		uint32_t GetMipLevelCount() const override;

		void Lock() override;
		void Unlock() override;

		void Resize(uint32_t width, uint32_t height) override;

		RendererID GetRendererID() const override { return mID; }
		const std::string& GetPath() const override { return mFilePath; }

		bool Loaded() const override { return mLoaded; }

		TextureFormat GetFormat() const override { return mFormat; }
		uint32_t GetWidth() const override { return mWidth; }
		uint32_t GetHeight() const override { return mHeight; }

		Buffer GetWriteableBuffer() override;

	private:
		RendererID mID;

		bool mLocked = false;
		bool mIsHDR = false;
		bool mLoaded = false;

		TextureFormat mFormat;
		uint32_t mWidth, mHeight;
		Buffer mImageData;
		TextureWrap mWrap = TextureWrap::Clamp;

		std::string mFilePath;
	};

	class GLTextureCube : public TextureCube
	{
	public:
		GLTextureCube(TextureFormat format, uint32_t width, uint32_t height);
		GLTextureCube(const std::string& path);
		~GLTextureCube() override;

		void Bind(uint32_t slot = 0) const override;

		RendererID GetRendererID() const override { return mID; }

		TextureFormat GetFormat() const override { return mFormat; }
		uint32_t GetWidth() const override { return mWidth; }
		uint32_t GetHeight() const override { return mHeight; }

		uint32_t GetMipLevelCount() const override;

		const std::string& GetPath() const override { return mFilePath; }

	private:
		RendererID mID;

		TextureFormat mFormat;
		TextureWrap mWrap = TextureWrap::Clamp;
		uint32_t mWidth, mHeight;
		unsigned char* mImageData;

		std::string mFilePath;
	};
}