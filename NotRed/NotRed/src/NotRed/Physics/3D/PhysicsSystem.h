#pragma once

#include "PhysicsMeshCache.h"

namespace NR
{
	class PhysicsSystem
	{
	public:
		static void Init();
		static void Shutdown();

		static PhysicsMeshCache& GetMeshCache() { return sPhysicsMeshCache; }

	private:
		inline static PhysicsMeshCache sPhysicsMeshCache;
	};
}