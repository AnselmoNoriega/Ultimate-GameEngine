#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Renderer/Renderer.h"

#include <string>
#include <glm/glm.hpp>

namespace NR
{
	struct NOT_RED_API ShaderUniform
	{

	};

	struct NOT_RED_API ShaderUniformCollection
	{

	};

	enum class NOT_RED_API UniformType
	{
		None,
		Float, Float2, Float3, Float4,
		Matrix3x3, Matrix4x4,
		Int32, Uint32
	};

	struct NOT_RED_API UniformDecl
	{
		UniformType Type;
		std::ptrdiff_t Offset;
		std::string Name;
	};

	struct NOT_RED_API UniformBuffer
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

		const byte* GetBuffer() const override { return Buffer; }
		const UniformDecl* GetUniforms() const override { return Uniforms; }
		unsigned int GetUniformCount() const { return U; }

		template<typename T>
		void Push(const std::string& name, const T& data) {}

		template<>
		void Push(const std::string& name, const float& data)
		{
			Uniforms[0] = { UniformType::Float, Cursor, name };
			memcpy(Buffer + Cursor, &data, sizeof(float));
			Cursor += sizeof(float);
		}

		template<>
		void Push(const std::string& name, const glm::vec4& data)
		{
			Uniforms[0] = { UniformType::Float4, Cursor, name };
			memcpy(Buffer + Cursor, glm::value_ptr(data), sizeof(glm::vec4));
			Cursor += sizeof(glm::vec4);
		}

	};

	class NOT_RED_API Shader
	{
	public:
		virtual void Bind() = 0;

		virtual void UploadUniformBuffer(const UniformBufferBase& uniformBuffer) = 0;

		static Shader* Create(const std::string& filepath);
	};

}