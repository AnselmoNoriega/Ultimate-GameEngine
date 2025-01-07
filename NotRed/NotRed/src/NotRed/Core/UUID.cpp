#include "nrpch.h"
#include "UUID.h"

#include <random>

namespace NR
{
	static std::random_device sRandomDevice;
	static std::mt19937_64 eng(sRandomDevice());
	static std::uniform_int_distribution<uint64_t> sUniformDistribution;

	static std::mt19937 eng32(sRandomDevice());
	static std::uniform_int_distribution<uint32_t> sUniformDistribution32;

	UUID::UUID()
		: mID(sUniformDistribution(eng))
	{
	}

	UUID::UUID(uint64_t uuid)
		: mID(uuid)
	{
	}

	UUID::UUID(const UUID& other)
		: mID(other.mID)
	{
	}


	UUID32::UUID32()
		: mUUID(sUniformDistribution32(eng32))
	{
	}

	UUID32::UUID32(uint32_t uuid)
		: mUUID(uuid)
	{
	}

	UUID32::UUID32(const UUID32& other)
		: mUUID(other.mUUID)
	{
	}
}