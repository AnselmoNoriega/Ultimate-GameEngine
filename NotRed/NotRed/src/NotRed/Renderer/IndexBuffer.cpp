#include "nrpch.h"
#include "IndexBuffer.h"

#include "Renderer.h"

#include "Platform/OpenGL/GLIndexBuffer.h"

namespace NR
{
    Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
            {
                NR_CORE_ASSERT(false, "RendererAPI \"None\" is currently not supported!") return nullptr;
            }

            case RendererAPI::API::OpenGL:
            {
                return CreateRef<GLIndexBuffer>(indices, count);
            }

            default:
            {
                NR_CORE_ASSERT(false, "Unknown Renderer API!");
                return nullptr;
            }
        }
    }
}