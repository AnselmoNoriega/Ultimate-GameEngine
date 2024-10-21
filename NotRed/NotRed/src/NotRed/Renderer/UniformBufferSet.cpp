#include "nrpch.h"
#include "UniformBufferSet.h"

#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Platform/OpenGL/GLUniformBufferSet.h"
#include "NotRed/Platform/Vulkan/VKUniformBufferSet.h"

namespace NR
{
    Ref<UniformBufferSet> UniformBufferSet::Create(uint32_t frames)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:     return nullptr;
        case RendererAPIType::Vulkan:  return Ref<VKUniformBufferSet>::Create(frames);
        case RendererAPIType::OpenGL:  return Ref<GLUniformBufferSet>::Create(frames);
        default:
            NR_CORE_ASSERT(false, "Unknown RendererAPI!");
            return nullptr;
        }
    }
}