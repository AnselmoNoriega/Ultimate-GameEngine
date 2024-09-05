#include "nrpch.h"
#include "PhysicsManager.h"

#include <extensions/PxBroadPhaseExt.h>

#include "NotRed/Script/ScriptEngine.h"

#include "PhysicsWrappers.h"
#include "PhysicsLayer.h"
#include "PhysicsActor.h"

namespace NR
{
	static physx::PxScene* sScene;
	static std::vector<Ref<PhysicsActor>> sActors;
	static float sSimulationTime = 0.0f;

	static PhysicsSettings sSettings;

	void PhysicsManager::Init()
	{
		PhysicsWrappers::Initialize();
		PhysicsLayerManager::AddLayer("Default");
	}

	void PhysicsManager::Shutdown()
	{
		PhysicsWrappers::Shutdown();
	}

	void PhysicsManager::CreateScene()
	{
		NR_CORE_ASSERT(sScene == nullptr, "Scene already has a Physics Scene!");
		sScene = PhysicsWrappers::CreateScene();

		if (sSettings.BroadphaseAlgorithm != BroadphaseType::AutomaticBoxPrune)
		{
			physx::PxBounds3* regionBounds;
			physx::PxBounds3 globalBounds(ToPhysicsVector(sSettings.WorldBoundsMin), ToPhysicsVector(sSettings.WorldBoundsMax));
			uint32_t regionCount = physx::PxBroadPhaseExt::createRegionsFromWorldBounds(regionBounds, globalBounds, sSettings.WorldBoundsSubdivisions);

			for (uint32_t i = 0; i < regionCount; ++i)
			{
				physx::PxBroadPhaseRegion region;
				region.mBounds = regionBounds[i];
				sScene->addBroadPhaseRegion(region);
			}
		}
	}

	Ref<PhysicsActor> PhysicsManager::CreateActor(Entity e)
	{
		NR_CORE_ASSERT(sScene);

		Ref<PhysicsActor> actor = Ref<PhysicsActor>::Create(e);

		sActors.push_back(actor);
		actor->Spawn();
		return actor;
	}

	Ref<PhysicsActor> PhysicsManager::GetActorForEntity(Entity entity)
	{
		for (auto& actor : sActors)
		{
			if (actor->GetEntity() == entity)
			{
				return actor;
			}
		}

		return nullptr;
	}

	PhysicsSettings& PhysicsManager::GetSettings()
	{
		return sSettings;
	}

	void PhysicsManager::Simulate(float dt)
	{
		sSimulationTime += dt;

		if (sSimulationTime < sSettings.FixedDeltaTime)
		{
			return;
		}

		sSimulationTime -= sSettings.FixedDeltaTime;

		for (auto& actor : sActors)
		{
			actor->Update(sSettings.FixedDeltaTime);
		}

		sScene->simulate(sSettings.FixedDeltaTime);
		sScene->fetchResults(true);

		for (auto& actor : sActors)
		{
			actor->SynchronizeTransform();
		}
	}

	void PhysicsManager::DestroyScene()
	{
		NR_CORE_ASSERT(sScene);

		sActors.clear();
		sScene->release();
		sScene = nullptr;
	}

	void* PhysicsManager::GetPhysicsScene()
	{
		return sScene;
	}
}