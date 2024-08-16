#pragma once

#include "NotRed/Renderer/ShaderUniform.h"

namespace NR
{
	class GLShaderResourceDeclaration : public ShaderResourceDeclaration
	{
	public:
		enum class Type
		{
			NONE, TEXTURE2D, TEXTURECUBE
		};
	private:
		friend class GLShader;
	private:
		std::string mName;
		uint32_t mRegister = 0;
		uint32_t mCount;
		Type mType;
	public:
		GLShaderResourceDeclaration(Type type, const std::string& name, uint32_t count);

		inline const std::string& GetName() const override { return mName; }
		inline uint32_t GetRegister() const override { return mRegister; }
		inline uint32_t GetCount() const override { return mCount; }

		inline Type GetType() const { return mType; }
	public:
		static Type StringToType(const std::string& type);
		static std::string TypeToString(Type type);
	};

	class GLShaderUniformDeclaration : public ShaderUniformDeclaration
	{
	private:
		friend class GLShader;
		friend class GLShaderUniformBufferDeclaration;
	public:
		enum class Type
		{
			NONE, FLOAT32, VEC2, VEC3, VEC4, MAT3, MAT4, INT32, STRUCT
		};
	private:
		std::string mName;
		uint32_t mSize;
		uint32_t mCount;
		uint32_t mOffset;
		ShaderDomain mDomain;

		Type mType;
		ShaderStruct* mStruct;
		mutable int32_t mLocation;
	public:
		GLShaderUniformDeclaration(ShaderDomain domain, Type type, const std::string& name, uint32_t count = 1);
		GLShaderUniformDeclaration(ShaderDomain domain, ShaderStruct* uniformStruct, const std::string& name, uint32_t count = 1);

		inline const std::string& GetName() const override { return mName; }
		inline uint32_t GetSize() const override { return mSize; }
		inline uint32_t GetCount() const override { return mCount; }
		inline uint32_t GetOffset() const override { return mOffset; }
		inline uint32_t GetAbsoluteOffset() const { return mStruct ? mStruct->GetOffset() + mOffset : mOffset; }
		inline ShaderDomain GetDomain() const { return mDomain; }

		int32_t GetLocation() const { return mLocation; }
		inline Type GetType() const { return mType; }
		inline bool IsArray() const { return mCount > 1; }
		inline const ShaderStruct& GetShaderUniformStruct() const { HZ_CORE_ASSERT(mStruct, ""); return *mStruct; }
	protected:
		void SetOffset(uint32_t offset) override;
	public:
		static uint32_t SizeOfUniformType(Type type);
		static Type StringToType(const std::string& type);
		static std::string TypeToString(Type type);
	};

	struct GLShaderUniformField
	{
		GLShaderUniformDeclaration::Type type;
		std::string name;
		uint32_t count;
		mutable uint32_t size;
		mutable int32_t location;
	};

	class GLShaderUniformBufferDeclaration : public ShaderUniformBufferDeclaration
	{
	private:
		friend class Shader;
	private:
		std::string mName;
		ShaderUniformList mUniforms;
		uint32_t mRegister;
		uint32_t mSize;
		ShaderDomain mDomain;
	public:
		GLShaderUniformBufferDeclaration(const std::string& name, ShaderDomain domain);

		void PushUniform(GLShaderUniformDeclaration* uniform);

		inline const std::string& GetName() const override { return mName; }
		inline uint32_t GetRegister() const override { return mRegister; }
		inline uint32_t GetSize() const override { return mSize; }
		virtual ShaderDomain GetDomain() const { return mDomain; }
		inline const ShaderUniformList& GetUniformDeclarations() const override { return mUniforms; }

		ShaderUniformDeclaration* FindUniform(const std::string& name);
	};

}