#pragma once

#include <glad/glad.h>
#include <spirv_cross/spirv_glsl.hpp>

#include "NotRed/Renderer/Shader.h"

namespace NR
{
	class GLShader : public Shader
	{
	public:
		GLShader() = default;
		GLShader(const std::string& filepath, bool forceCompile);
		static Ref<GLShader> CreateFromString(const std::string& vertSrc, const std::string& fragSrc, const std::string& computeSrc);

		void Bind();
		RendererID GetRendererID() const { return mID; }

		void Reload(bool forceCompile = false) override;

		size_t GetHash() const override;

		void SetUniformBuffer(const std::string& name, const void* data, uint32_t size);
		void SetUniform(const std::string& fullname, uint32_t value);
		void SetUniform(const std::string& fullname, int value);
		void SetUniform(const std::string& fullname, float value);
		void SetUniform(const std::string& fullname, const glm::vec2& value);
		void SetUniform(const std::string& fullname, const glm::vec3& value);
		void SetUniform(const std::string& fullname, const glm::vec4& value);
		void SetUniform(const std::string& fullname, const glm::mat3& value);
		void SetUniform(const std::string& fullname, const glm::mat4& value);
		void SetIntArray(const std::string& name, int* values, uint32_t size);

		void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

		const std::string& GetName() const override { return mName; }

		const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const override { return mBuffers; }
		const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const override { return mResources; }

		const ShaderResourceDeclaration* GetShaderResource(const std::string& name);

		static void ClearUniformBuffers();

	private:
		void Load(const std::string& vertSrc, const std::string& fragSrc, const std::string& computeSrc, bool forceCompile);
		void Compile(const std::vector<uint32_t>& vertexBinary, const std::vector<uint32_t>& fragmentBinary);
		void Reflect(std::vector<uint32_t>& data);

		void CompileOrGetVulkanBinary(std::unordered_map<uint32_t, std::vector<uint32_t>>& outputBinary, bool forceCompile = false);
		void CompileOrGetOpenGLBinary(const std::unordered_map<uint32_t, std::vector<uint32_t>>&, bool forceCompile = false);

		void ParseFile(const std::string& filepath, std::string& output, bool isCompute = false) const;

		void ParseConstantBuffers(const spirv_cross::CompilerGLSL& compiler);
		int32_t GetUniformLocation(const std::string& name) const;

		void UploadUniformUInt(const std::string& name, uint32_t value);
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

	private:
		RendererID mID = 0;
		std::string mName, mAssetPath;

		std::unordered_map<uint32_t, std::string> mShaderSource;
		bool mLoaded = false;
		bool mIsCompute = false;

		uint32_t mConstantBufferOffset = 0;

		std::vector<ShaderReloadedCallback> mShaderReloadedCallbacks;

		std::unordered_map<std::string, ShaderBuffer> mBuffers;
		std::unordered_map<std::string, ShaderResourceDeclaration> mResources;
		std::unordered_map<std::string, GLint> mUniformLocations;

		inline static std::unordered_map<uint32_t, ShaderUniformBuffer> sUniformBuffers;
	};
}