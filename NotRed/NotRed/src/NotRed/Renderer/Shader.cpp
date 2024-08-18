#include "nrpch.h"
#include "Shader.h"

#include "NotRed/Platform/OpenGL/GLShader.h"

namespace NR
{
    std::vector<Ref<Shader>> Shader::sAllShaders;

    Ref<Shader> Shader::Create(const std::string& filepath)
    {
        Ref<Shader> result = nullptr;

        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
        case RendererAPIType::OpenGL: result = std::make_shared<GLShader>(filepath);
        }

        sAllShaders.push_back(result);
        return result;
    }

	Ref<Shader> Shader::CreateFromString(const std::string& vertSrc, const std::string& fragSrc)
	{
		Ref<Shader> result = nullptr;

		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: result = GLShader::CreateFromString(vertSrc, fragSrc);
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

	void ShaderLibrary::Load(const std::string& path)
	{
		auto shader = Ref<Shader>(Shader::Create(path));
		auto& name = shader->GetName();
		NR_CORE_ASSERT(mShaders.find(name) == mShaders.end());
		mShaders[name] = shader;
	}

	void ShaderLibrary::Load(const std::string& name, const std::string& path)
	{
		NR_CORE_ASSERT(mShaders.find(name) == mShaders.end());
		mShaders[name] = Ref<Shader>(Shader::Create(path));
	}

	Ref<Shader>& ShaderLibrary::Get(const std::string& name)
	{
		NR_CORE_ASSERT(mShaders.find(name) != mShaders.end());
		return mShaders[name];
	}
}