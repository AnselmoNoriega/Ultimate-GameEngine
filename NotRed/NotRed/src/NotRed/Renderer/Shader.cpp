#include "nrpch.h"
#include "Shader.h"

#include "NotRed/Platform/OpenGL/GLShader.h"

namespace NR
{
    std::vector<Shader*> Shader::sAllShaders;

    Shader* Shader::Create(const std::string& filepath)
    {
        Shader* result = nullptr;

        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
        case RendererAPIType::OpenGL: return new GLShader(filepath);
        }

        sAllShaders.push_back(result);
        return result;
    }
}