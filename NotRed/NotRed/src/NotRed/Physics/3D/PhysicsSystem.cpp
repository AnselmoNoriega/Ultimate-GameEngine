#include "nrpch.h"
#include "PhysicsSystem.h"

namespace NR
{
	void PhysicsSystem::Init()
	{
		sPhysicsMeshCache.Init();
	}

	void PhysicsSystem::Shutdown()
	{
		sPhysicsMeshCache.Clear();
	}
}