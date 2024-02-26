#include "nrpch.h"
#include "VertexArray.h"

#include "Renderer.h"

#include "Platform/OpenGL/GLVertexArray.h"

namespace NR
{
    VertexArray* VertexArray::Create()
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: NR_CORE_ASSERT(false, "Renderer API \"None\" is currently not supported!");
        case RendererAPI::API::OpenGL: return new GLVertexArray();
        }
    }
}