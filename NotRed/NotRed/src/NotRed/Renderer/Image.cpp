#include "nrpch.h"
#include "Image.h"

#include "NotRed/Platform/Vulkan/VKImage.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
    Ref<Image2D> Image2D::Create(ImageSpecification specification, Buffer buffer)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:		return nullptr;
        case RendererAPIType::Vulkan:	return Ref<VKImage2D>::Create(specification);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }

    Ref<Image2D> Image2D::Create(ImageSpecification specification, const void* data)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
        case RendererAPIType::Vulkan: return Ref<VKImage2D>::Create(specification);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }
}