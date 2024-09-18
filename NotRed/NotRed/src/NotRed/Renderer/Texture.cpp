#include "nrpch.h"
#include "Texture.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Platform/OpenGL/GLTexture.h"
#include "NotRed/Platform/Vulkan/VKTexture.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
	Ref<Texture2D> Texture2D::Create(ImageFormat format, uint32_t width, uint32_t height, const void* data)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return Ref<GLTexture2D>::Create(format, width, height, data);
		case RendererAPIType::Vulkan: return Ref<VKTexture2D>::Create(format, width, height, data);
        default:
			NR_CORE_ASSERT(false, "Unknown RendererAPI");
			return nullptr;
        }
    }

	Ref<Texture2D> Texture2D::Create(const std::string& path, bool standardRGB)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return Ref<GLTexture2D>::Create(path, standardRGB);
		case RendererAPIType::Vulkan: return Ref<VKTexture2D>::Create(path, standardRGB);
		default:
		{
			return nullptr;
		}
		}
	}

	Ref<TextureCube> TextureCube::Create(ImageFormat format, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return Ref<GLTextureCube>::Create(format, width, height, data);
		case RendererAPIType::Vulkan: return Ref<VKTextureCube>::Create(format, width, height, data);
		default:
			NR_CORE_ASSERT(false, "Unknown RendererAPI");
			return nullptr;
		}
	}

	Ref<TextureCube> TextureCube::Create(const std::string& path)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return Ref<GLTextureCube>::Create(path);
		default:
			NR_CORE_ASSERT(false, "Unknown RendererAPI");
			return nullptr;
		}
	}
}