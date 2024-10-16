#include "NRpch.h"
#include "RenderCommandBuffer.h"

#include "NotRed/Platform/Vulkan/VKRenderCommandBuffer.h"
#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
    Ref<RenderCommandBuffer> RenderCommandBuffer::Create(uint32_t count, const std::string& debugName)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:    return nullptr;
        case RendererAPIType::Vulkan:  return Ref<VKRenderCommandBuffer>::Create(count, debugName);
        default:
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
    }

    Ref<RenderCommandBuffer> RenderCommandBuffer::CreateFromSwapChain(const std::string& debugName)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:    return nullptr;
        case RendererAPIType::Vulkan:  return Ref<VKRenderCommandBuffer>::Create(debugName, true);
        default:
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
    }
}