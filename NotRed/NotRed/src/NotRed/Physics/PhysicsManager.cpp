#include "nrpch.h"
#include "PhysicsManager.h"

#include "PhysicsInternal.h"
#include "PhysicsLayer.h"

namespace NR
{
	void PhysicsManager::Init()
	{
		PhysicsInternal::Initialize();
		PhysicsLayerManager::AddLayer("Default");
	}

	void PhysicsManager::Shutdown()
	{
		PhysicsInternal::Shutdown();
	}

	void PhysicsManager::Simulate(float dt)
	{
		sScene->Simulate(dt);
	}

	void PhysicsManager::RuntimePlay()
	{
		sScene = Ref<PhysicsScene>::Create(sSettings);
		PhysicsInternal::StartDebugger("Physics DebugInfo");
	}

	void PhysicsManager::RuntimeStop()
	{
		PhysicsInternal::StopDebugger();
		sScene->Destroy();
	}

	PhysicsSettings PhysicsManager::sSettings;
	Ref<PhysicsScene> PhysicsManager::sScene;

}