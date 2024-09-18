#include "nrpch.h"
#include "Image.h"

#include "NotRed/Platform/Vulkan/VKImage.h"
#include "NotRed/Platform/OpenGL/GLImage.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
    Ref<Image2D> Image2D::Create(ImageFormat format, uint32_t width, uint32_t height, Buffer buffer)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:		return nullptr;
        case RendererAPIType::OpenGL:	return Ref<GLImage2D>::Create(format, width, height, buffer);
        case RendererAPIType::Vulkan:	return Ref<VKImage2D>::Create(format, width, height);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }

    Ref<Image2D> Image2D::Create(ImageFormat format, uint32_t width, uint32_t height, const void* data)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
        case RendererAPIType::OpenGL: return Ref<GLImage2D>::Create(format, width, height, data);
        case RendererAPIType::Vulkan: return Ref<VKImage2D>::Create(format, width, height);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }
}