#pragma once

#include "NotRed/Core/Core.h"
#include "RendererAPI.h"

namespace NR
{
	enum class NOT_RED_API TextureFormat
	{
		None,
		RGB,
		RGBA
	};

	class NOT_RED_API Texture
	{
	public:
		virtual ~Texture() {}

		virtual RendererID GetRendererID() const = 0;
	};

	class NOT_RED_API Texture2D : public Texture
	{
	public:
		static Texture2D* Create(TextureFormat format, uint32_t width, uint32_t height);
		static Texture2D* Create(const std::string& path, bool standardRGB = false);

		virtual void Bind(unsigned int slot = 0) const = 0;

		virtual TextureFormat GetFormat() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual const std::string& GetPath() const = 0;
	};

	class TextureCube : public Texture
	{
	public:
		static TextureCube* Create(const std::string& path);

		virtual void Bind(unsigned int slot = 0) const = 0;

		virtual TextureFormat GetFormat() const = 0;
		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		virtual const std::string& GetPath() const = 0;
	};
}