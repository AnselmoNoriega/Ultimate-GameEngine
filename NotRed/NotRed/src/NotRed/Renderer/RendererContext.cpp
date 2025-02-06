#include "nrpch.h"
#include "RendererContext.h"

#include "NotRed/Renderer/RendererAPI.h"

#include "NotRed/Platform/Vulkan/VKContext.h"

namespace NR
{
    Ref<RendererContext> RendererContext::Create()
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:    return nullptr;
        case RendererAPIType::Vulkan:  return Ref<VKContext>::Create();
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }
}