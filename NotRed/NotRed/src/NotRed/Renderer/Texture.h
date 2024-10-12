#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Buffer.h"

#include "NotRed/Asset/Asset.h"
#include "NotRed/Renderer/Image.h"

namespace NR
{
	class Texture : public Asset
	{
	public:
		virtual ~Texture() {}

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual ImageFormat GetFormat() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevelCount() const = 0;

		virtual uint64_t GetHash() const = 0;

		virtual TextureType GetType() const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(ImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr, TextureProperties properties = TextureProperties());
		static Ref<Texture2D> Create(const std::string& path, TextureProperties properties = TextureProperties());

		virtual Ref<Image2D> GetImage() const = 0;

		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual bool Loaded() const = 0;

		virtual Buffer GetWriteableBuffer() = 0;

		virtual const std::string& GetPath() const = 0;
		TextureType GetType() const override { return TextureType::Texture2D; }

		static AssetType GetStaticType() { return AssetType::Texture; }
		AssetType GetAssetType() const override { return AssetType::Texture; }
	};

	class TextureCube : public Texture
	{
	public:
		static Ref<TextureCube> Create(ImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr, TextureProperties properties = TextureProperties());
		static Ref<TextureCube> Create(const std::string& path, TextureProperties properties = TextureProperties());

		virtual const std::string& GetPath() const = 0;
		TextureType GetType() const override { return TextureType::TextureCube; }

		static AssetType GetStaticType() { return AssetType::EnvMap; }
		AssetType GetAssetType() const override { return AssetType::EnvMap; }
	};
}