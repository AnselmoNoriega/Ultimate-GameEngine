#include "nrpch.h"
#include "PhysicsManager.h"

#include "PhysicsInternal.h"
#include "PhysicsLayer.h"
#include "Debug/PhysicsDebugger.h"

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

	void PhysicsManager::RuntimePlay()
	{
		sScene = Ref<PhysicsScene>::Create(sSettings);	
		
		if (sSettings.DebugOnPlay)
		{
			PhysicsDebugger::StartDebugging("PhysicsDebugInfo");
		}
	}

	void PhysicsManager::RuntimeStop()
	{
		if (sSettings.DebugOnPlay)
		{
			PhysicsDebugger::StopDebugging();
		}
		sScene->Destroy();
	}

	PhysicsSettings PhysicsManager::sSettings;
	Ref<PhysicsScene> PhysicsManager::sScene;

}