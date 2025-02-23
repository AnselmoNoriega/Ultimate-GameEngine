#include "nrpch.h"
#include "PhysicsSystem.h"

namespace NR
{
	void PhysicsSystem::Init()
	{
	}

	void PhysicsSystem::Shutdown()
	{
		sPhysicsMeshCache.Clear();
	}
}