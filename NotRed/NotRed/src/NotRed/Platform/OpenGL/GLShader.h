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

		//void Reload() override;

		void Bind() override;

		void UploadUniformBuffer(const UniformBufferBase& uniformBuffer) override;

		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) override;

		virtual const std::string& GetName() const override { return mName; }

	private:
		std::string ParseFile(const std::string& filepath);
		std::string GetShaderName(const std::string& filepath);
		void Compile(const ShaderInfo* shaders, uint32_t count);

		void UploadUniformInt(const std::string& name, int value);

		void UploadUniformFloat(const std::string& name, float value);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& values);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& values);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& values);

		void UploadUniformMat4(const std::string& name, const glm::mat4& values);

	private:
		RendererID mID;

		std::string mName;
	};
}