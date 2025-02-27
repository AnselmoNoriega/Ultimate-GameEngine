#include "nrpch.h"
#include "PhysicsManager.h"

#include "PhysicsInternal.h"
#include "PhysicsLayer.h"
#include "Debug/PhysicsDebugger.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Project/Project.h"

#include "NotRed/Debug/Profiler.h"

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

	void PhysicsManager::ScenePlay()
	{
		NR_PROFILE_FUNC();

#ifdef NR_DEBUG
		if (sSettings.DebugOnPlay && !PhysicsDebugger::IsDebugging())
		{
			PhysicsDebugger::StartDebugging(
				(Project::GetActive()->GetProjectDirectory() / "PhysicsDebugInfo").string(), 
				PhysicsManager::GetSettings().DebugType == DebugType::LiveDebug
			);
		}
#endif		
	}

	void PhysicsManager::SceneStop()
	{
#ifdef NR_DEBUG
		if (sSettings.DebugOnPlay)
		{
			PhysicsDebugger::StopDebugging();
		}
#endif
	}

	PhysicsSettings PhysicsManager::sSettings;
}