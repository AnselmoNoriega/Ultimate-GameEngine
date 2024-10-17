#pragma once

#include "NotRed/Renderer/RendererTypes.h"
#include "NotRed/Renderer/Texture.h"

namespace NR
{
	class GLTexture2D : public Texture2D
	{
	public:
		GLTexture2D(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties);
		GLTexture2D(const std::string& path, TextureProperties properties);
		~GLTexture2D() override;

		void Bind(uint32_t slot = 0) const override;

		uint32_t GetMipLevelCount() const override;

		void Lock() override;
		void Unlock() override;

		const std::string& GetPath() const override { return mFilePath; }

		bool Loaded() const override { return mLoaded; }

		Ref<Image2D> GetImage() const override { return mImage; }
		ImageFormat GetFormat() const override { return mImage->GetSpecification().Format; }
		uint32_t GetWidth() const override { return mWidth; }
		uint32_t GetHeight() const override { return mHeight; }

		uint64_t GetHash() const override { return mImage->GetHash(); }

		Buffer GetWriteableBuffer() override;

	private:
		Ref<Image2D> mImage;
		TextureProperties mProperties;

		bool mLocked = false;
		bool mIsHDR = false;
		bool mLoaded = false;

		uint32_t mWidth, mHeight;
		Buffer mImageData;

		std::string mFilePath;
	};

	class GLTextureCube : public TextureCube
	{
	public:
		GLTextureCube(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties);
		GLTextureCube(const std::string& path, TextureProperties properties);
		~GLTextureCube() override;

		void Bind(uint32_t slot = 0) const override;

		RendererID GetRendererID() const { return mID; }
		uint64_t GetHash() const override { return (uint64_t)mID; }

		ImageFormat GetFormat() const override { return mFormat; }
		uint32_t GetWidth() const override { return mWidth; }
		uint32_t GetHeight() const override { return mHeight; }

		uint32_t GetMipLevelCount() const override;

#if NEEDED
		const std::string& GetPath() const override { return mFilePath; }
#endif
	private:
		RendererID mID;

		ImageFormat mFormat;
		uint32_t mWidth, mHeight;

		Buffer mLocalStorage;
		TextureProperties mProperties;

		std::string mFilePath;
	};
}