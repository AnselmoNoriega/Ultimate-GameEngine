#include "nrpch.h"
#include "GLShaderUniform.h"

namespace NR
{
	GLShaderUniformDeclaration::GLShaderUniformDeclaration(ShaderDomain domain, Type type, const std::string& name, uint32_t count)
		: mType(type), mStruct(nullptr), mDomain(domain)
	{
		mName = name;
		mCount = count;
		mSize = SizeOfUniformType(type) * count;
	}

	GLShaderUniformDeclaration::GLShaderUniformDeclaration(ShaderDomain domain, ShaderStruct* uniformStruct, const std::string& name, uint32_t count)
		: mStruct(uniformStruct), mType(GLShaderUniformDeclaration::Type::STRUCT), mDomain(domain)
	{
		mName = name;
		mCount = count;
		mSize = mStruct->GetSize() * count;
	}

	void GLShaderUniformDeclaration::SetOffset(uint32_t offset)
	{
		if (mType == GLShaderUniformDeclaration::Type::STRUCT)
			mStruct->SetOffset(offset);

		mOffset = offset;
	}

	uint32_t GLShaderUniformDeclaration::SizeOfUniformType(Type type)
	{
		switch (type)
		{
		case GLShaderUniformDeclaration::Type::INT32:      return 4;
		case GLShaderUniformDeclaration::Type::FLOAT32:    return 4;
		case GLShaderUniformDeclaration::Type::VEC2:       return 4 * 2;
		case GLShaderUniformDeclaration::Type::VEC3:       return 4 * 3;
		case GLShaderUniformDeclaration::Type::VEC4:       return 4 * 4;
		case GLShaderUniformDeclaration::Type::MAT3:       return 4 * 3 * 3;
		case GLShaderUniformDeclaration::Type::MAT4:       return 4 * 4 * 4;
		}
		return 0;
	}

	GLShaderUniformDeclaration::Type GLShaderUniformDeclaration::StringToType(const std::string& type)
	{
		if (type == "int32")    return Type::INT32;
		if (type == "float")    return Type::FLOAT32;
		if (type == "vec2")     return Type::VEC2;
		if (type == "vec3")     return Type::VEC3;
		if (type == "vec4")     return Type::VEC4;
		if (type == "mat3")     return Type::MAT3;
		if (type == "mat4")     return Type::MAT4;

		return Type::NONE;
	}

	std::string GLShaderUniformDeclaration::TypeToString(Type type)
	{
		switch (type)
		{
		case GLShaderUniformDeclaration::Type::INT32:      return "int32";
		case GLShaderUniformDeclaration::Type::FLOAT32:    return "float";
		case GLShaderUniformDeclaration::Type::VEC2:       return "vec2";
		case GLShaderUniformDeclaration::Type::VEC3:       return "vec3";
		case GLShaderUniformDeclaration::Type::VEC4:       return "vec4";
		case GLShaderUniformDeclaration::Type::MAT3:       return "mat3";
		case GLShaderUniformDeclaration::Type::MAT4:       return "mat4";
		}
		return "Invalid Type";
	}

	GLShaderUniformBufferDeclaration::GLShaderUniformBufferDeclaration(const std::string& name, ShaderDomain domain)
		: mName(name), mDomain(domain), mSize(0), mRegister(0)
	{
	}

	void GLShaderUniformBufferDeclaration::PushUniform(GLShaderUniformDeclaration* uniform)
	{
		uint32_t offset = 0;
		if (mUniforms.size())
		{
			GLShaderUniformDeclaration* previous = (GLShaderUniformDeclaration*)mUniforms.back();
			offset = previous->mOffset + previous->mSize;
		}
		uniform->SetOffset(offset);
		mSize += uniform->GetSize();
		mUniforms.push_back(uniform);
	}

	ShaderUniformDeclaration* GLShaderUniformBufferDeclaration::FindUniform(const std::string& name)
	{
		for (ShaderUniformDeclaration* uniform : mUniforms)
		{
			if (uniform->GetName() == name)
				return uniform;
		}
		return nullptr;
	}

	GLShaderResourceDeclaration::GLShaderResourceDeclaration(Type type, const std::string& name, uint32_t count)
		: mType(type), mName(name), mCount(count)
	{
		mName = name;
		mCount = count;
	}

	GLShaderResourceDeclaration::Type GLShaderResourceDeclaration::StringToType(const std::string& type)
	{
		if (type == "sampler2D")		return Type::TEXTURE2D;
		if (type == "samplerCube")		return Type::TEXTURECUBE;

		return Type::NONE;
	}

	std::string GLShaderResourceDeclaration::TypeToString(Type type)
	{
		switch (type)
		{
		case Type::TEXTURE2D:	return "sampler2D";
		case Type::TEXTURECUBE:		return "samplerCube";
		}
		return "Invalid Type";
	}

}