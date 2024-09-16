#include "nrpch.h"

#include "NotRed/Platform/OpenGL/GLVertexBuffer.h"
#include "NotRed/Platform/Vulkan/VkVertexBuffer.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
    Ref<VertexBuffer> VertexBuffer::Create(void* data, uint32_t size, VertexBufferUsage usage)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:    return nullptr;
        case RendererAPIType::OpenGL:  return Ref<GLVertexBuffer>::Create(data, size, usage);
        case RendererAPIType::Vulkan:  return Ref<VkVertexBuffer>::Create(data, size, usage);
        default:
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
    }

    Ref<VertexBuffer> VertexBuffer::Create(uint32_t size, VertexBufferUsage usage)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:    return nullptr;
        case RendererAPIType::OpenGL:  return Ref<GLVertexBuffer>::Create(size, usage);
        case RendererAPIType::Vulkan:  return Ref<VkVertexBuffer>::Create(size, usage);
        default:
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
    }
}
