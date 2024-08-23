#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Log.h"

#include <string>
#include <vector>

namespace NR
{
	enum class ShaderDomain
	{
		None, Vertex, Pixel
	};

	class ShaderUniformDeclaration
	{
	public:
		virtual const std::string& GetName() const = 0;
		virtual uint32_t GetSize() const = 0;
		virtual uint32_t GetCount() const = 0;
		virtual uint32_t GetOffset() const = 0;
		virtual ShaderDomain GetDomain() const = 0;

	private:
		friend class Shader;
		friend class GLShader;
		friend class ShaderStruct;

	protected:
		virtual void SetOffset(uint32_t offset) = 0;
	};

	typedef std::vector<ShaderUniformDeclaration*> ShaderUniformList;

	class ShaderUniformBufferDeclaration : public RefCounted
	{
	public:
		virtual const std::string& GetName() const = 0;
		virtual uint32_t GetRegister() const = 0;
		virtual uint32_t GetSize() const = 0;
		virtual const ShaderUniformList& GetUniformDeclarations() const = 0;

		virtual ShaderUniformDeclaration* FindUniform(const std::string& name) = 0;
	};

	typedef std::vector<ShaderUniformBufferDeclaration*> ShaderUniformBufferList;

	class ShaderStruct
	{
	public:
		ShaderStruct(const std::string& name)
			: mName(name), mSize(0), mOffset(0) { }

		void AddField(ShaderUniformDeclaration* field)
		{
			mSize += field->GetSize();
			uint32_t offset = 0;
			if (mFields.size())
			{
				ShaderUniformDeclaration* previous = mFields.back();
				offset = previous->GetOffset() + previous->GetSize();
			}
			field->SetOffset(offset);
			mFields.push_back(field);
		}

		inline void SetOffset(uint32_t offset) { mOffset = offset; }

		inline const std::string& GetName() const { return mName; }
		inline uint32_t GetSize() const { return mSize; }
		inline uint32_t GetOffset() const { return mOffset; }
		inline const std::vector<ShaderUniformDeclaration*>& GetFields() const { return mFields; }

	private:
		friend class Shader;

	private:
		std::string mName;
		std::vector<ShaderUniformDeclaration*> mFields;
		uint32_t mSize;
		uint32_t mOffset;
	};

	typedef std::vector<ShaderStruct*> ShaderStructList;

	class ShaderResourceDeclaration
	{
	public:
		virtual const std::string& GetName() const = 0;
		virtual uint32_t GetRegister() const = 0;
		virtual uint32_t GetCount() const = 0;
	};

	typedef std::vector<ShaderResourceDeclaration*> ShaderResourceList;

}