#include "nrpch.h"
#include "RendererContext.h"

#include "NotRed/Renderer/RendererAPI.h"

#include "NotRed/Platform/OpenGL/GLContext.h"
#include "NotRed/Platform/Vulkan/VKContext.h"

namespace NR
{
    Ref<RendererContext> RendererContext::Create(GLFWwindow* windowHandle)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:    return nullptr;
        case RendererAPIType::OpenGL:  return Ref<GLContext>::Create(windowHandle);
        case RendererAPIType::Vulkan:  return Ref<VKContext>::Create(windowHandle);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }
}