#include "nrpch.h"
#include "Shader.h"

#include "NotRed/Platform/OpenGL/GLShader.h"

namespace NR
{
    Shader* Shader::Create(const std::string& filepath)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
        case RendererAPIType::OpenGL: return new OpenGLShader(filepath);
        default:
        {
            return nullptr;
        }
        }
    }
}