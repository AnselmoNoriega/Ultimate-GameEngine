#include "nrpch.h"
#include "UUID.h"

#include <random>

namespace NR
{
	static std::random_device sRandomDevice;
	static std::mt19937_64 eng(sRandomDevice());
	static std::uniform_int_distribution<uint64_t> sUniformDistribution;

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
}