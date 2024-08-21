#include "nrpch.h"
#include "Texture.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Platform/OpenGL/GLTexture.h"

namespace NR
{
	Ref<Texture2D> Texture2D::Create(TextureFormat format, uint32_t width, uint32_t height, TextureWrap wrap)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
        case RendererAPIType::OpenGL: return CreateRef<GLTexture2D>(format, width, height, wrap);
        default:
        {
            return nullptr;
        }
        }
    }

	Ref<Texture2D> Texture2D::Create(const std::string& path, bool standardRGB)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return CreateRef<GLTexture2D>(path, standardRGB);
		default:
		{
			return nullptr;
		}
		}
	}

	Ref<TextureCube> TextureCube::Create(TextureFormat format, uint32_t width, uint32_t height)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return CreateRef<GLTextureCube>(format, width, height);
		default:
		{
			return nullptr;
		}
		}
	}

	Ref<TextureCube> TextureCube::Create(const std::string& path)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return CreateRef<GLTextureCube>(path);
		default:
		{
			return nullptr;
		}
		}
	}

	uint32_t Texture::GetBPP(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGB:    return 3;
		case TextureFormat::RGBA:   return 4;
		}
		return 0;
	}

	uint32_t Texture::CalculateMipMapCount(uint32_t width, uint32_t height)
	{
		uint32_t levels = 1;
		while ((width | height) >> levels)
		{
			levels++;
		}

		return levels;
	}
}