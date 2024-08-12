#include "nrpch.h"
#include "Texture.h"

#include "Renderer.h"
#include "Platform/OpenGL/GLTexture.h"

namespace NR
{
    Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: NR_CORE_ASSERT(false, "Renderer API \"None\" is currently not supported!");
        case RendererAPI::API::OpenGL: return CreateRef<GLTexture2D>(specification);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RenderAPI!");
            return nullptr;
        }
        }

        return nullptr;
    }

    Ref<Texture2D> Texture2D::Create(const std::string& path)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: NR_CORE_ASSERT(false, "Renderer API \"None\" is currently not supported!");
        case RendererAPI::API::OpenGL: return CreateRef<GLTexture2D>(path);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RenderAPI!");
            return nullptr;
        }
        }

        return nullptr;
    }

    Ref<TextureCube> TextureCube::Create(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
    {
        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::None: return nullptr;
        case RendererAPI::API::OpenGL: return Ref<OpenGLTextureCube>::Create(format, width, height, data, properties);
        //case RendererAPIType::Vulkan: return Ref<VkTextureCube>::Create(format, width, height, data);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RenderAPI!");
            return nullptr;
        }
        }
        return nullptr;
    }

    Ref<TextureCube> TextureCube::Create(const std::string& path, TextureProperties properties)
    {
        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::None: return nullptr;
        case RendererAPI::API::OpenGL: return Ref<GLTextureCube>::Create(path, properties);
        //case RendererAPI::API::Vulkan: return Ref<VulkanTextureCube>::Create(path, properties);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RenderAPI!");
            return nullptr;
        }
        }

        return nullptr;
    }
}