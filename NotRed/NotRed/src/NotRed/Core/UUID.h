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

	class UUID32
	{
	public:
		UUID32();
		UUID32(uint32_t uuid);
		UUID32(const UUID32& other);

		operator uint32_t () { return mUUID; }
		operator const uint32_t() const { return mUUID; }
	private:
		uint32_t mUUID;
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

	template <>
	struct hash<NR::UUID32>
	{
		std::size_t operator()(const NR::UUID32& uuid) const
		{
			return hash<uint32_t>()((uint32_t)uuid);
		}
	};
}

inline std::ostream& operator<<(std::ostream& os, const NR::UUID& uuid)
{
	return os << std::hex << uuid;
}