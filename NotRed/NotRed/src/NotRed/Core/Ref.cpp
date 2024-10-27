#include "nrpch.h"

#include <unordered_set>

namespace NR
{
	static std::unordered_set<void*> sLiveReferences;
	static std::mutex sLiveReferenceMutex;

	namespace RefUtils 
	{
		void AddToLiveReferences(void* instance)
		{
			std::scoped_lock<std::mutex> lock(sLiveReferenceMutex);
			NR_CORE_ASSERT(instance);
			sLiveReferences.insert(instance);
		}

		void RemoveFromLiveReferences(void* instance)
		{
			std::scoped_lock<std::mutex> lock(sLiveReferenceMutex);
			NR_CORE_ASSERT(instance);
			NR_CORE_ASSERT(sLiveReferences.find(instance) != sLiveReferences.end());
			sLiveReferences.erase(instance);
		}

		bool IsLive(void* instance)
		{
			NR_CORE_ASSERT(instance);
			return sLiveReferences.find(instance) != sLiveReferences.end();
		}
	}
}