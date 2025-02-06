#include "nrpch.h"
#include "Texture.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Platform/Vulkan/VKTexture.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
	Ref<Texture2D> Texture2D::Create(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
		case RendererAPIType::Vulkan: return Ref<VKTexture2D>::Create(format, width, height, data, properties);
        default:
			NR_CORE_ASSERT(false, "Unknown RendererAPI");
			return nullptr;
        }
    }

	Ref<Texture2D> Texture2D::Create(const std::string& path, TextureProperties properties)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::Vulkan: return Ref<VKTexture2D>::Create(path, properties);
		default:
		{
			return nullptr;
		}
		}
	}

	Ref<TextureCube> TextureCube::Create(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::Vulkan: return Ref<VKTextureCube>::Create(format, width, height, data, properties);
		default:
			NR_CORE_ASSERT(false, "Unknown RendererAPI");
			return nullptr;
		}
	}

	Ref<TextureCube> TextureCube::Create(const std::string& path, TextureProperties properties)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::Vulkan: return Ref<VKTextureCube>::Create(path, properties);
		default:
			NR_CORE_ASSERT(false, "Unknown RendererAPI");
			return nullptr;
		}
	}
}