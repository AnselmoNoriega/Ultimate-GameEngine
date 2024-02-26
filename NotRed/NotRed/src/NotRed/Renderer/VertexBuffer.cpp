#include "nrpch.h"
#include "VertexBuffer.h"

#include "Renderer.h"

#include "Platform/OpenGL/GLVertexBuffer.h"

namespace NR
{
    VertexBuffer* VertexBuffer::Create(float* vertices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
            {
                NR_CORE_ASSERT(false, "RendererAPI \"None\" is currently not supported!") return nullptr;
            }

            case RendererAPI::API::OpenGL:
            {
                return new GLVertexBuffer(vertices, size);
            }

            default:
            {
                NR_CORE_ASSERT(false, "Unknown Renderer API!");
                return nullptr;
            }
        }
    }
}
