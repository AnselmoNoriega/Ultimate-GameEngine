#include "nrpch.h"
#include "IndexBuffer.h"

#include "NotRed/Platform/Vulkan/VKIndexBuffer.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
    Ref<IndexBuffer> IndexBuffer::Create(void* data, uint32_t size)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:    return nullptr;
        case RendererAPIType::Vulkan:  return Ref<VKIndexBuffer>::Create(data, size);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }

    Ref<IndexBuffer> IndexBuffer::Create(uint32_t size)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:    return nullptr;
        case RendererAPIType::Vulkan:  return Ref<VKIndexBuffer>::Create(size);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }
}