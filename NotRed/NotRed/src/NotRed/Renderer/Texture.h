#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Buffer.h"

#include "NotRed/Asset/Asset.h"
#include "NotRed/Renderer/Image.h"

namespace NR
{
	enum class TextureWrap
	{
		None,
		Clamp,
		Repeat
	};

	enum class TextureType
	{
		None,
		Texture2D,
		TextureCube
	};

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
	public:		static Ref<Texture2D> Create(ImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
		static Ref<Texture2D> Create(const std::string& path, bool standardRGB = false);

		virtual Ref<Image2D> GetImage() const = 0;

		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual bool Loaded() const = 0;

		virtual Buffer GetWriteableBuffer() = 0;

		virtual const std::string& GetPath() const = 0;
		TextureType GetType() const override { return TextureType::Texture2D; }
	};

	class TextureCube : public Texture
	{
	public:
		static Ref<TextureCube> Create(ImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
		static Ref<TextureCube> Create(const std::string& path);

		virtual const std::string& GetPath() const = 0;
		TextureType GetType() const override { return TextureType::TextureCube; }
	};
}