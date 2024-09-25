#pragma once

#include "Core.h"
#include <xhash>

namespace NR
{
	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID& other);

		operator uint64_t() { return mID; }
		operator const uint64_t() const { return mID; }

	private:
		uint64_t mID;
	};
}

namespace std 
{
	template <>
	struct hash<NR::UUID>
	{
		std::size_t operator()(const NR::UUID& uuid) const
		{
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};
}