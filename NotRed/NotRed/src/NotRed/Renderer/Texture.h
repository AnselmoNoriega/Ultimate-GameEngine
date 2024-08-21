#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Buffer.h"
#include "RendererAPI.h"

namespace NR
{
	enum class TextureFormat
	{
		None,
		RGB,
		RGBA,
		Float16
	};

	enum class TextureWrap
	{
		None,
		Clamp,
		Repeat
	};

	class Texture
	{
	public:
		virtual ~Texture() {}

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual TextureFormat GetFormat() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevelCount() const = 0;

		virtual RendererID GetRendererID() const = 0;

		static uint32_t GetBPP(TextureFormat format);
		static uint32_t CalculateMipMapCount(uint32_t width, uint32_t height);

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(TextureFormat format, uint32_t width, uint32_t height, TextureWrap wrap = TextureWrap::Clamp);
		static Ref<Texture2D> Create(const std::string& path, bool standardRGB = false);

		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual bool Loaded() const = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual Buffer GetWriteableBuffer() = 0;

		virtual const std::string& GetPath() const = 0;
	};

	class TextureCube : public Texture
	{
	public:
		static Ref<TextureCube> Create(TextureFormat format, uint32_t width, uint32_t height);
		static Ref<TextureCube> Create(const std::string& path);

		virtual const std::string& GetPath() const = 0;
	};
}