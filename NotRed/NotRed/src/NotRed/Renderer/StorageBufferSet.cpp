
#include "nrpch.h"
#include "UniformBufferSet.h"

#include "StorageBufferSet.h"

#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Platform/Vulkan/VKStorageBufferSet.h"
#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
    Ref<StorageBufferSet> StorageBufferSet::Create(uint32_t frames)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:     return nullptr;
        case RendererAPIType::Vulkan:   return Ref<VulkanStorageBufferSet>::Create(frames);
        default:
            NR_CORE_ASSERT(false, "Unknown RendererAPI!");
            return nullptr;
        }
    }
}