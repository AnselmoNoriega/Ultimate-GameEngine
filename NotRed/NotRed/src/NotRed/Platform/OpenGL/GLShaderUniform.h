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

	public:
		GLShaderResourceDeclaration(Type type, const std::string& name, uint32_t count);

		inline const std::string& GetName() const override { return mName; }
		inline uint32_t GetRegister() const override { return mRegister; }
		inline uint32_t GetCount() const override { return mCount; }

		inline Type GetType() const { return mType; }

		static Type StringToType(const std::string& type);
		static std::string TypeToString(Type type);

	private:
		friend class GLShader;

	private:
		std::string mName;
		uint32_t mRegister = 0;
		uint32_t mCount;
		Type mType;
	};

	class GLShaderUniformDeclaration : public ShaderUniformDeclaration
	{
	public:
		enum class Type
		{
			NONE, 
			FLOAT32, 
			VEC2, VEC3, VEC4, 
			MAT3, MAT4, 
			INT32, 
			STRUCT
		};

	public:
		GLShaderUniformDeclaration(ShaderDomain domain, Type type, const std::string& name, uint32_t count = 1);
		GLShaderUniformDeclaration(ShaderDomain domain, ShaderStruct* uniformStruct, const std::string& name, uint32_t count = 1);

		inline const std::string& GetName() const override { return mName; }
		inline uint32_t GetSize() const override { return mSize; }
		inline uint32_t GetCount() const override { return mCount; }
		inline uint32_t GetOffset() const override { return mOffset; }
		inline ShaderDomain GetDomain() const { return mDomain; }
		inline uint32_t GetAbsoluteOffset() const { return mStruct ? mStruct->GetOffset() + mOffset : mOffset; }

		int32_t GetLocation() const { return mLocation; }
		inline Type GetType() const { return mType; }

		inline bool IsArray() const { return mCount > 1; }
		inline const ShaderStruct& GetShaderUniformStruct() const { NR_CORE_ASSERT(mStruct, ""); return *mStruct; }

	public:
		static uint32_t SizeOfUniformType(Type type);
		static Type StringToType(const std::string& type);
		static std::string TypeToString(Type type);

	protected:
		void SetOffset(uint32_t offset) override;

	private:
		friend class GLShader;
		friend class GLShaderUniformBufferDeclaration;

	private:
		std::string mName;
		uint32_t mSize;
		uint32_t mCount;
		uint32_t mOffset;
		ShaderDomain mDomain;

		Type mType;
		ShaderStruct* mStruct;
		mutable int32_t mLocation;
	};

	struct GLShaderUniformField
	{
		std::string name;
		uint32_t count;
		mutable uint32_t size;
		mutable int32_t location;
		GLShaderUniformDeclaration::Type type;
	};

	class GLShaderUniformBufferDeclaration : public ShaderUniformBufferDeclaration
	{
	public:
		GLShaderUniformBufferDeclaration(const std::string& name, ShaderDomain domain);

		void PushUniform(GLShaderUniformDeclaration* uniform);

		inline const std::string& GetName() const override { return mName; }
		inline uint32_t GetSize() const override { return mSize; }
		inline uint32_t GetRegister() const override { return mRegister; }
		virtual ShaderDomain GetDomain() const { return mDomain; }
		inline const ShaderUniformList& GetUniformDeclarations() const override { return mUniforms; }

		ShaderUniformDeclaration* FindUniform(const std::string& name);

	private:
		friend class Shader;

	private:
		std::string mName;
		uint32_t mSize;
		uint32_t mRegister;
		ShaderDomain mDomain;
		ShaderUniformList mUniforms;
	};

}