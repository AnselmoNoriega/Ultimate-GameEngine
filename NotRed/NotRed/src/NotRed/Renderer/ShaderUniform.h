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

	class ShaderResourceDeclaration
	{
	public:
		ShaderResourceDeclaration() = default;
		ShaderResourceDeclaration(const std::string& name, uint32_t resourceRegister, uint32_t count)
			: mName(name), mRegister(resourceRegister), mCount(count) {}

		virtual const std::string& GetName() const { return mName; }
		virtual uint32_t GetRegister() const { return mRegister; }
		virtual uint32_t GetCount() const { return mCount; }
	private:
		std::string mName;
		uint32_t mRegister = 0;
		uint32_t mCount = 0;
	};

	typedef std::vector<ShaderResourceDeclaration*> ShaderResourceList;

}