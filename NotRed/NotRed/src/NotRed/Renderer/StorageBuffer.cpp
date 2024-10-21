#include "nrpch.h"
#include "StorageBuffer.h"

#include "NotRed/Platform/Vulkan/VKStorageBuffer.h"
#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
    Ref<StorageBuffer> StorageBuffer::Create(uint32_t size, uint32_t binding)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:     return nullptr;
        case RendererAPIType::Vulkan:   return Ref<VKStorageBuffer>::Create(size, binding);
        default:
            NR_CORE_ASSERT(false, "Unknown RendererAPI!");
            return nullptr;
        }
    }
}