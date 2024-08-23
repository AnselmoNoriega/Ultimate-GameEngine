#pragma once

#include "NotRed/Renderer/Shader.h"
#include <glad/glad.h>

#include "GLShaderUniform.h"

namespace NR
{
	class GLShader : public Shader
	{
	public:
		GLShader() = default;
		GLShader(const std::string& filepath);
		static Ref<GLShader> CreateFromString(const std::string& vertSrc, const std::string& fragSrc);

		void Bind() override;
		RendererID GetRendererID() const override { return mID; }

		void Reload() override;

		void UploadUniformBuffer(const UniformBufferBase& uniformBuffer) override;

		void SetVSMaterialUniformBuffer(Buffer buffer) override;
		void SetPSMaterialUniformBuffer(Buffer buffer) override;

		void SetInt(const std::string& name, int value) override;
		void SetFloat(const std::string& name, float value) override;
		void SetFloat3(const std::string& name, const glm::vec3& value) override;
		void SetMat4(const std::string& name, const glm::mat4& value) override;
		void SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind = true) override;
		void SetIntArray(const std::string& name, int* values, uint32_t size) override;

		void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

		const std::string& GetName() const override { return mName; }

	private:
		void Load(const std::string& vertSrc, const std::string& fragSrc, const std::string& computeSrc = "");

		std::string GetShaderName(const std::string& filepath);

		void ParseFile(const std::string& filepath, std::string& output, bool isCompute = false);
		void Parse(const std::string& source, ShaderDomain type);
		void ParseUniform(const std::string& statement, ShaderDomain domain);
		void ParseUniformStruct(const std::string& block, ShaderDomain domain);
		
		ShaderStruct* FindStruct(const std::string& name);
		int32_t GetUniformLocation(const std::string& name) const;

		void Compile();
		void ResolveUniforms();
		void ValidateUniforms();

		void ResolveAndSetUniform(GLShaderUniformDeclaration* uniform, Buffer buffer);
		void ResolveAndSetUniforms(const Ref<GLShaderUniformBufferDeclaration>& decl, Buffer buffer);
		void ResolveAndSetUniformArray(GLShaderUniformDeclaration* uniform, Buffer buffer);
		void ResolveAndSetUniformField(const GLShaderUniformDeclaration& field, byte* data, int32_t offset);

		void UploadUniformInt(uint32_t location, int32_t value);
		void UploadUniformInt(const std::string& name, int32_t value);

		void UploadUniformIntArray(uint32_t location, int32_t* values, int32_t count);
		void UploadUniformIntArray(const std::string& name, int32_t* values, uint32_t count);

		void UploadUniformFloat(uint32_t location, float value);
		void UploadUniformFloat(const std::string& name, float value);

		void UploadUniformFloat2(uint32_t location, const glm::vec2& value);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& value);

		void UploadUniformFloat3(uint32_t location, const glm::vec3& value);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& value);

		void UploadUniformFloat4(uint32_t location, const glm::vec4& value);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& value);

		void UploadUniformMat3(uint32_t location, const glm::mat3& values);

		void UploadUniformMat4(uint32_t location, const glm::mat4& values);
		void UploadUniformMat4Array(uint32_t location, const glm::mat4& values, uint32_t count);
		void UploadUniformMat4(const std::string& name, const glm::mat4& value);

		void UploadUniformStruct(GLShaderUniformDeclaration* uniform, byte* buffer, uint32_t offset);
		
		const ShaderUniformBufferList& GetVSRendererUniforms() const override { return mVSRendererUniformBuffers; }
		const ShaderUniformBufferList& GetPSRendererUniforms() const override { return mPSRendererUniformBuffers; }
		bool HasVSMaterialUniformBuffer() const override { return (bool)mVSMaterialUniformBuffer; }
		bool HasPSMaterialUniformBuffer() const override { return (bool)mPSMaterialUniformBuffer; }
		const ShaderUniformBufferDeclaration& GetVSMaterialUniformBuffer() const override { return *mVSMaterialUniformBuffer; }
		const ShaderUniformBufferDeclaration& GetPSMaterialUniformBuffer() const override { return *mPSMaterialUniformBuffer; }
		const ShaderResourceList& GetResources() const override { return mResources; }

	private:
		RendererID mID = 0;
		std::string mName, mAssetPath;

		std::unordered_map<uint32_t, std::string> mShaderSource;
		bool mLoaded = false;
		bool mIsCompute = false;

		std::vector<ShaderReloadedCallback> mShaderReloadedCallbacks;

		ShaderUniformBufferList mVSRendererUniformBuffers;
		ShaderUniformBufferList mPSRendererUniformBuffers;
		Ref<GLShaderUniformBufferDeclaration> mVSMaterialUniformBuffer;
		Ref<GLShaderUniformBufferDeclaration> mPSMaterialUniformBuffer;
		ShaderResourceList mResources;
		ShaderStructList mStructs;
	};
}