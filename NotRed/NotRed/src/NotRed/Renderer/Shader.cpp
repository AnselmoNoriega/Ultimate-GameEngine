#include "nrpch.h"
#include "Shader.h"

#include "Renderer.h"
#include "Platform/OpenGL/GLShader.h"

namespace NR
{
    Shader* Shader::Create(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None:   NR_CORE_ASSERT(false, "Renderer API \"None\" is currently not supported!");
        case RendererAPI::API::OpenGL: return new GLShader(vertexSrc, fragmentSrc);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RenderAPI!");
            return nullptr;
        }
        }
    }
}