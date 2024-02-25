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
        case RendererAPI::None: NR_CORE_ASSERT(false, "Renderer API \"None\" is currently not supported!");
        case RendererAPI::OpenGL: return new GLVertexArray();
        }
    }
}