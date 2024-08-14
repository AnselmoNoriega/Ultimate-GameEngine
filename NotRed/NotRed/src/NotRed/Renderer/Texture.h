#pragma once

#include "NotRed/Core/Core.h"

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
	};

	class NOT_RED_API Texture2D : public Texture
	{
	public:
		static Texture2D* Create(TextureFormat format, uint32_t width, uint32_t height);

		virtual TextureFormat GetFormat() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
	};
}