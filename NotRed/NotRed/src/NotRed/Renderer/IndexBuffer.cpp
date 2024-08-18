#include "nrpch.h"
#include "IndexBuffer.h"

#include "NotRed/Platform/OpenGL/GLIndexBuffer.h"

namespace NR
{
    Ref<IndexBuffer> IndexBuffer::Create(void* data, uint32_t size)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:    return nullptr;
        case RendererAPIType::OpenGL:  return std::make_shared<GLIndexBuffer>(data, size);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }
}