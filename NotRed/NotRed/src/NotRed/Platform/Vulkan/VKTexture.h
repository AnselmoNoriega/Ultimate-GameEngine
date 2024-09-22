#pragma once

#include "NotRed/Renderer/Texture.h"

#include "Vulkan.h"

#include "VKImage.h"

namespace NR
{
	class VKTexture2D : public Texture2D
	{
	public:
		VKTexture2D(const std::string& path, bool srgb = false);
		VKTexture2D(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureWrap wrap = TextureWrap::Clamp);
		~VKTexture2D() override;

		void Invalidate();

		ImageFormat GetFormat() const override { return mFormat; }
		uint32_t GetWidth() const override { return mWidth; }
		uint32_t GetHeight() const override { return mHeight; }

		void Bind(uint32_t slot = 0) const override;

		Ref<Image2D> GetImage() const override { return mImage; }
		const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return mImage.As<VKImage2D>()->GetDescriptor(); }

		void Lock() override;
		void Unlock() override;

		Buffer GetWriteableBuffer() override;
		bool Loaded() const override;
		const std::string& GetPath() const override;
		uint32_t GetMipLevelCount() const override;

		void GenerateMips();

		uint64_t GetHash() const override { return (uint64_t)mImage; }

	private:
		std::string mPath;
		uint32_t mWidth;
		uint32_t mHeight;

		Buffer mImageData;

		Ref<Image2D> mImage;

		ImageFormat mFormat = ImageFormat::None;
	};

	class VKTextureCube : public TextureCube
	{
	public:
		VKTextureCube(ImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
		VKTextureCube(const std::string& path);
		~VKTextureCube() override;

		const std::string& GetPath() const override { return ""; }

		void Bind(uint32_t slot = 0) const override {}

		ImageFormat GetFormat() const { return mFormat; }

		uint32_t GetWidth() const override { return mWidth; }
		uint32_t GetHeight() const override { return mHeight; }
		uint32_t GetMipLevelCount() const override;

		uint64_t GetHash() const override { return (uint64_t)mImage; }

		const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return mDescriptorImageInfo; }
		VkImageView CreateImageViewSingleMip(uint32_t mip);

		void GenerateMips(bool readonly = false);

	private:
		void Invalidate();

	private:
		ImageFormat mFormat = ImageFormat::None;
		uint32_t mWidth = 0, mHeight = 0;

		bool mMipsGenerated = false;

		Buffer mLocalStorage;
		VmaAllocation mMemoryAlloc;
		VkImage mImage;
		VkDescriptorImageInfo mDescriptorImageInfo = {};
	};
}