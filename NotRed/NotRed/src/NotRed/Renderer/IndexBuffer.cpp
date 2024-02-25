#include "nrpch.h"
#include "IndexBuffer.h"

#include "Renderer.h"

#include "Platform/OpenGL/GLIndexBuffer.h"

namespace NR
{
    IndexBuffer* IndexBuffer::Create(uint32_t* indices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::None:
            {
                NR_CORE_ASSERT(false, "RendererAPI \"None\" is currently not supported!") return nullptr;
            }

            case RendererAPI::OpenGL:
            {
                return new GLIndexBuffer(indices, size);
            }

            default:
            {
                NR_CORE_ASSERT(false, "Unknown Renderer API!");
                return nullptr;
            }
        }
    }
}