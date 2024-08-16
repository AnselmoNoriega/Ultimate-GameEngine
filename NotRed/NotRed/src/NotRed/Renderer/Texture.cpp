#include "nrpch.h"
#include "Texture.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Platform/OpenGL/GLTexture.h"

namespace NR
{
    Texture2D* Texture2D::Create(TextureFormat format, uint32_t width, uint32_t height, TextureWrap wrap)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
        case RendererAPIType::OpenGL: return new GLTexture2D(format, width, height, wrap);
        default:
        {
            return nullptr;
        }
        }
    }

	Texture2D* Texture2D::Create(const std::string& path, bool standardRGB)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return new GLTexture2D(path, standardRGB);
		}
		return nullptr;
	}

	TextureCube* TextureCube::Create(const std::string& path)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return new GLTextureCube(path);
		}
		return nullptr;
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
}