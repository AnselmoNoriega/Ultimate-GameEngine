#include "nrpch.h"
#include "Shader.h"

#include "NotRed/Platform/OpenGL/GLShader.h"
#include "NotRed/Platform/Vulkan/VkShader.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
    std::vector<Ref<Shader>> Shader::sAllShaders;

    Ref<Shader> Shader::Create(const std::string& filepath, bool forceCompile)
    {
        Ref<Shader> result = nullptr;

        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
        case RendererAPIType::OpenGL:
            result = Ref<GLShader>::Create(filepath, forceCompile);
            break;
        case RendererAPIType::Vulkan:
            result = Ref<VkShader>::Create(filepath, forceCompile);
            break;
        }

        sAllShaders.push_back(result);
        return result;
    }

    Ref<Shader> Shader::CreateFromString(const std::string& vertSrc, const std::string& fragSrc, const std::string& computeSrc)
    {
        Ref<Shader> result = nullptr;

        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
        case RendererAPIType::OpenGL: result = GLShader::CreateFromString(vertSrc, fragSrc, computeSrc);
        }

        sAllShaders.push_back(result);
        return result;
    }

    ShaderLibrary::ShaderLibrary()
    {
    }

    ShaderLibrary::~ShaderLibrary()
    {
    }

    void ShaderLibrary::Add(const Ref<Shader>& shader)
    {
        auto& name = shader->GetName();
        NR_CORE_ASSERT(mShaders.find(name) == mShaders.end());
        mShaders[name] = shader;
    }

    void ShaderLibrary::Load(const std::string& path, bool forceCompile)
    {
        auto shader = Ref<Shader>(Shader::Create(path, forceCompile));
        auto& name = shader->GetName();
        NR_CORE_ASSERT(mShaders.find(name) == mShaders.end());
        mShaders[name] = shader;
    }

    void ShaderLibrary::Load(const std::string& name, const std::string& path)
    {
        NR_CORE_ASSERT(mShaders.find(name) == mShaders.end());
        mShaders[name] = Shader::Create(path);
    }

    const Ref<Shader>& ShaderLibrary::Get(const std::string& name) const
    {
        NR_CORE_ASSERT(mShaders.find(name) != mShaders.end());
        return mShaders.at(name);
    }

    ShaderUniform::ShaderUniform(const std::string& name, ShaderUniformType type, uint32_t size, uint32_t offset)
        : mName(name), mType(type), mSize(size), mOffset(offset)
    {
    }

    const std::string& ShaderUniform::UniformTypeToString(ShaderUniformType type)
    {
        if (type == ShaderUniformType::Bool)
        {
            return "Boolean";
        }
        else if (type == ShaderUniformType::Int)
        {
            return "Int";
        }
        else if (type == ShaderUniformType::Float)
        {
            return "Float";
        }

        return "None";
    }
}