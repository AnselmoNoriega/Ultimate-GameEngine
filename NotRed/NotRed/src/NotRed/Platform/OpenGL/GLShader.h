#pragma once

#include "NotRed/Renderer/Shader.h"
#include <Glad/glad.h>

namespace NR
{
	class NOT_RED_API GLShader : public Shader
	{
		using ShaderInfo = std::pair<uint32_t, std::string>;

	public:
		GLShader(const std::string& filepath);

		void Bind() override;

		void UploadUniformBuffer(const UniformBufferBase& uniformBuffer) override;

	private:
		std::string ParseFile(const std::string& filepath);
		std::string GetShaderName(const std::string& filepath);
		void Compile(const ShaderInfo* shaders, uint32_t count);

		void UploadUniformFloat(const std::string& name, float value);
		void UploadUniformFloat2(const std::string& name, float* values);
		void UploadUniformFloat3(const std::string& name, float* values);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& values);

	private:
		RendererID mID;

		std::string mShaderSource;
		std::string mName;
	};
}