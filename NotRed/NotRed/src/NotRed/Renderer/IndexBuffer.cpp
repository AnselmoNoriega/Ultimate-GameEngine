#include "nrpch.h"
#include "IndexBuffer.h"

#include "Renderer.h"

#include "Platform/OpenGL/GLIndexBuffer.h"

namespace NR
{
    Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
            {
                NR_CORE_ASSERT(false, "RendererAPI \"None\" is currently not supported!") return nullptr;
            }

            case RendererAPI::API::OpenGL:
            {
                return CreateRef<GLIndexBuffer>(indices, size);
            }

            default:
            {
                NR_CORE_ASSERT(false, "Unknown Renderer API!");
                return nullptr;
            }
        }
    }
}