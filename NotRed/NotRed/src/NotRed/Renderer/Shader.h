#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Renderer/RendererTypes.h"
#include "NotRed/Core/Buffer.h"
#include "NotRed/Renderer/ShaderUniform.h"

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace NR
{
	enum class UniformType
	{
		None,
		Float, Float2, Float3, Float4,
		Matrix3x3, Matrix4x4,
		Int32, Uint32
	};

	struct UniformDecl
	{
		UniformType Type;
		std::ptrdiff_t Offset;
		std::string Name;
	};

	struct UniformBuffer
	{
		byte* Buffer;
		std::vector<UniformDecl> Uniforms;
	};

	struct UniformBufferBase
	{
		virtual const byte* GetBuffer() const = 0;
		virtual const UniformDecl* GetUniforms() const = 0;
		virtual unsigned int GetUniformCount() const = 0;
	};

	template<uint32_t N, uint32_t U>
	struct UniformBufferDeclaration : public UniformBufferBase
	{
		byte Buffer[N];
		UniformDecl Uniforms[U];
		std::ptrdiff_t Cursor = 0;
		int Index = 0;

		const byte* GetBuffer() const override { return Buffer; }
		const UniformDecl* GetUniforms() const override { return Uniforms; }
		unsigned int GetUniformCount() const { return U; }

		template<typename T>
		void Push(const std::string& name, const T& data) {}

		template<>
		void Push(const std::string& name, const float& data)
		{
			Uniforms[Index++] = { UniformType::Float, Cursor, name };
			memcpy(Buffer + Cursor, &data, sizeof(float));
			Cursor += sizeof(float);
		}

		template<>
		void Push(const std::string& name, const glm::vec3& data)
		{
			Uniforms[Index++] = { UniformType::Float3, Cursor, name };
			memcpy(Buffer + Cursor, glm::value_ptr(data), sizeof(glm::vec3));
			Cursor += sizeof(glm::vec3);
		}

		template<>
		void Push(const std::string& name, const glm::vec4& data)
		{
			Uniforms[Index++] = { UniformType::Float4, Cursor, name };
			memcpy(Buffer + Cursor, glm::value_ptr(data), sizeof(glm::vec4));
			Cursor += sizeof(glm::vec4);
		}

		template<>
		void Push(const std::string& name, const glm::mat4& data)
		{
			Uniforms[Index++] = { UniformType::Matrix4x4, Cursor, name };
			memcpy(Buffer + Cursor, glm::value_ptr(data), sizeof(glm::mat4));
			Cursor += sizeof(glm::mat4);
		}
	};

	enum class ShaderUniformType
	{
		None, 
		Bool, Int, UInt, Float, 
		Vec2, Vec3, Vec4, 
		Mat3, Mat4
	};

	class ShaderUniform
	{
	public:
		ShaderUniform() = default;
		ShaderUniform(const std::string& name, ShaderUniformType type, uint32_t size, uint32_t offset);

		static const std::string& UniformTypeToString(ShaderUniformType type);

		const std::string& GetName() const { return mName; }
		ShaderUniformType GetType() const { return mType; }
		uint32_t GetSize() const { return mSize; }
		uint32_t GetOffset() const { return mOffset; }

	private:
		std::string mName;
		ShaderUniformType mType = ShaderUniformType::None;
		uint32_t mSize = 0;
		uint32_t mOffset = 0;
	};

	struct ShaderUniformBuffer
	{
		std::string Name;
		uint32_t Index;
		uint32_t BindingPoint;
		uint32_t Size;
		uint32_t RendererID;
		std::vector<ShaderUniform> Uniforms;
	};

	struct ShaderBuffer
	{
		std::string Name;
		uint32_t Size = 0;
		std::unordered_map<std::string, ShaderUniform> Uniforms;
	};

	class Shader : public RefCounted
	{
	public:
		using ShaderReloadedCallback = std::function<void()>;

		static Ref<Shader> Create(const std::string& filepath, bool forceCompile = false);
		Ref<Shader> CreateFromString(const std::string& vertSrc, const std::string& fragSrc, const std::string& computeSrc = "");

		virtual void Bind() = 0;
		virtual RendererID GetRendererID() const = 0;
		virtual void Reload(bool forceCompile = false) = 0;

		virtual size_t GetHash() const = 0;

		virtual void SetUniformBuffer(const std::string& name, const void* data, uint32_t size) = 0;
		virtual void SetUniform(const std::string& fullname, float value) = 0;
		virtual void SetUniform(const std::string& fullname, uint32_t value) = 0;
		virtual void SetUniform(const std::string& fullname, int value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::vec2& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::vec3& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::vec4& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::mat3& value) = 0;
		virtual void SetUniform(const std::string& fullname, const glm::mat4& value) = 0;

		virtual void SetInt(const std::string& name, int value) = 0;
		virtual void SetUInt(const std::string& name, uint32_t value) = 0;
		virtual void SetFloat(const std::string& name, float value) = 0;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;
		virtual void SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind = true) = 0;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t size) = 0;

		virtual const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const = 0;
		virtual const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const = 0;

		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) = 0;

		virtual const std::string& GetName() const = 0;

		static std::vector<Ref<Shader>> sAllShaders;
	};

	class ShaderLibrary : public RefCounted
	{
	public:
		ShaderLibrary();
		~ShaderLibrary();

		void Add(const Ref<Shader>& shader);
		void Load(const std::string& path, bool forceCompile = false);
		void Load(const std::string& name, const std::string& path);

		const Ref<Shader>& Get(const std::string& name) const;

	private:
		std::unordered_map<std::string, Ref<Shader>> mShaders;
	};

}